#!/usr/bin/env python3
"""Host checks; use a Python with numpy, OpenCV and pyserial installed."""
import contextlib
import ctypes
import importlib.util
import io
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import nn_uart as uart
import send_images as sender
import image_preprocessing as prep
import cv2
import numpy as np


class FakeSerial:
    """Fragmented UART replies from a small model-selection simulator."""
    def __init__(self, *args, **kwargs):
        self.buffer = bytearray()
        self.active = uart.NONE
        self.selections = []
        self.calls = []
        self.timeout = 0.2
        self.damage = None
        self.firmware_revision = 2
        self.timer_ticks = 10

    def __enter__(self): return self
    def __exit__(self, *args): pass
    def reset_input_buffer(self): self.buffer.clear()
    def flush(self): pass

    def write(self, data):
        request = uart.decode_packet(data)
        self.calls.append(request)
        status = 0
        if request.command == uart.INFO:
            model = uart.NONE
            payload = struct.pack('<III', self.firmware_revision, 1000000, self.timer_ticks)
        elif request.command == uart.STATUS:
            model = self.active
            payload = bytes([uart.READY if model else uart.UNSELECTED])
        elif request.command == uart.SELECT:
            self.active = model = request.model
            self.selections.append(model)
            payload = struct.pack('<I', 12345)
        else:
            model = request.model
            assert model == self.active and len(request.payload) == uart.INPUT_BYTES[model]
            payload = uart.RESULT.pack(2, 123, -32768, 32767, -1, 1)
        if self.damage == 'error': status, payload = 5, b''
        sequence = request.sequence + (self.damage == 'sequence')
        reply = uart.encode_packet(request.command, model, sequence, payload, status, response=True)
        if self.damage == 'crc': reply = reply[:-1] + bytes([reply[-1] ^ 1])
        if self.damage != 'timeout': self.buffer.extend(b'boot noise\xa5' + reply)
        return len(data)

    def read(self, length):
        count = min(length, 3, len(self.buffer))
        data = bytes(self.buffer[:count]); del self.buffer[:count]
        return data


class NativeChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        folder = Path(cls.temp.name)
        source = ROOT / 'hello_world/src'
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                        str(source / 'nn_protocol.c'), '-o', str(folder / 'protocol.so')], check=True)
        cls.lib = ctypes.CDLL(str(folder / 'protocol.so'))
        cls.lib.nn_crc16.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
        cls.lib.nn_crc16.restype = ctypes.c_uint16
        cls.lib.nn_validate.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
        cls.lib.nn_validate.restype = ctypes.c_int
        cls.lib.nn_logit16.argtypes = [ctypes.c_uint32, ctypes.c_uint]
        cls.lib.nn_logit16.restype = ctypes.c_int32
        # Use the real current BSP's register bit masks in the hardware fake.
        shutil.copy2(ROOT / 'platform/export/platform/sw/standalone_ps7_cortexa9_0/include/xdevcfg_hw.h', folder)
        shutil.copy2(ROOT / 'platform/export/platform/sw/standalone_ps7_cortexa9_0/include/xds_cnn_hw.h', folder)
        subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT / 'tests/fake_bsp'), '-I' + str(folder), '-I' + str(source),
                        str(source / 'dfx_runtime.c'), str(source / 'nn_protocol.c'),
                        str(ROOT / 'tests/test_runtime.c'), '-o', str(folder / 'runtime')], check=True)
        cls.runtime = folder / 'runtime'
        # Compile copies of the real BSP sources so their local header includes
        # resolve to the fake timer/MMIO interface, without changing the BSP.
        bsp = ROOT / 'platform/ps7_cortexa9_0/standalone_ps7_cortexa9_0/bsp/libsrc/xiltimer/src'
        for path in (bsp / 'xiltimer.c', bsp / 'core/default_timer/globaltimer_sleep_zynq.c'):
            shutil.copy2(path, folder)
        cls.timer = folder / 'timer_test'
        subprocess.run(['cc', '-std=c11', '-DSDT', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT / 'tests/timer_bsp'), '-I' + str(ROOT / 'tests/fake_bsp'),
                        str(source / 'platform.c'), str(folder / 'xiltimer.c'),
                        str(folder / 'globaltimer_sleep_zynq.c'), str(ROOT / 'tests/test_timer.c'),
                        '-o', str(cls.timer)], check=True)

    @classmethod
    def tearDownClass(cls): cls.temp.cleanup()

    def test_runtime_success_and_failure_paths(self):
        subprocess.run([self.runtime], check=True)

    def test_real_bsp_timer_starts_before_measurements(self):
        subprocess.run([self.timer], check=True)

    def test_crc_reference_and_both_payload_sizes(self):
        self.assertEqual(self.lib.nn_crc16(b'123456789', 9), 0x29b1)
        for model, count in uart.INPUT_BYTES.items():
            data = bytes((i % 256 for i in range(count)))
            packet = uart.encode_packet(uart.INFER, model, 0x1234, data)
            self.assertEqual(uart.decode_packet(packet).payload, data)
            body = packet[2:]
            self.assertEqual(self.lib.nn_validate(body, len(body)), 0)
            self.assertEqual(self.lib.nn_crc16(body[:-2], len(body) - 2), uart.crc16(body[:-2]))

    def test_c_rejects_bad_crc_length_and_version(self):
        packet = uart.encode_packet(uart.SELECT, uart.CNN, 1)[2:]
        damaged = packet[:-1] + bytes([packet[-1] ^ 1])
        self.assertEqual(self.lib.nn_validate(damaged, len(damaged)), 4)
        self.assertEqual(self.lib.nn_validate(packet, len(packet) - 1), 3)
        body = bytes([2]) + packet[1:-2]
        bad_version = body + struct.pack('<H', uart.crc16(body))
        self.assertEqual(self.lib.nn_validate(bad_version, len(bad_version)), 9)
        body = packet[:6] + struct.pack('<H', 9601)
        self.assertEqual(self.lib.nn_validate(body + b'\0\0', 10), 3)

    def test_c_signed_logit_unpacking(self):
        self.assertEqual(self.lib.nn_logit16(0x7fff8000, 0), -32768)
        self.assertEqual(self.lib.nn_logit16(0x7fff8000, 1), 32767)
        self.assertEqual(self.lib.nn_logit16(0x0001ffff, 0), -1)


class SenderChecks(unittest.TestCase):
    def test_firmware_and_running_timer_are_verified(self):
        port = FakeSerial(); client = uart.Client(port)
        self.assertEqual(client.timer_info(), (2, 1000000, 10))
        port.timer_ticks = 0
        with self.assertRaisesRegex(uart.ProtocolError, 'not advancing'):
            client.timer_info()
        port.timer_ticks = 10; port.firmware_revision = 1
        with self.assertRaisesRegex(uart.ProtocolError, 'expected 2'):
            client.timer_info()
        port.firmware_revision = 2; port.damage = 'error'
        with self.assertRaisesRegex(uart.ProtocolError, 'download'):
            client.timer_info()

    def test_status_switch_and_fragmented_inference_reply(self):
        port = FakeSerial(); client = uart.Client(port)
        self.assertEqual(client.status(), (uart.UNSELECTED, uart.NONE))
        for model in (uart.CNN, uart.SNN, uart.CNN):
            self.assertEqual(client.select(model), 12345)
            self.assertEqual(client.status(), (uart.READY, model))
            self.assertEqual(client.infer(model, bytes(uart.INPUT_BYTES[model])),
                             (2, 123, (-32768, 32767, -1, 1)))
        self.assertEqual(port.selections, [uart.CNN, uart.SNN, uart.CNN])

    def test_bad_replies_abort_without_retry(self):
        for damage in ('crc', 'sequence', 'error', 'timeout'):
            port = FakeSerial(); port.damage = damage
            client = uart.Client(port, timeout=0.001)
            with self.subTest(damage=damage), self.assertRaises(uart.ProtocolError): client.status()
            self.assertEqual(len(port.calls), 1)

    def test_payload_validation_before_sending(self):
        port = FakeSerial(); client = uart.Client(port)
        for model, payload in [(uart.CNN, bytes(768)), (99, bytes(768))]:
            with self.assertRaises(ValueError): client.infer(model, payload)
        self.assertFalse(port.calls)

    def test_wire_lengths_and_sequence_wrap(self):
        client = uart.Client(FakeSerial()); client.sequence = 65535
        client.status(); self.assertEqual(client.sequence, 0)
        with self.assertRaises(ValueError): uart.encode_packet(uart.INFER, uart.CNN, 1, bytes(9601))
        with self.assertRaises(uart.ProtocolError): uart.decode_packet(b'garbage')

    def test_both_uses_same_images_and_writes_two_reports(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); dataset = root / 'data'; output = root / 'out'
            for label, name in enumerate(sender.CLASSES):
                folder = dataset / name; folder.mkdir(parents=True)
                for index in range(2):
                    image = np.full((60, 80), label * 60, dtype=np.uint8)
                    cv2.rectangle(image, (5, 5), (30 + index, 40), 255, 2)
                    self.assertTrue(cv2.imwrite(str(folder / f'{index}.png'), image))
            port = FakeSerial()
            with patch('serial.Serial', return_value=port), patch.object(sender.time, 'sleep'), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(sender.main(['--model', 'both', '--dataset', str(dataset),
                                              '--output', str(output), '--limit', '1']), 0)
            self.assertEqual(port.selections, [uart.CNN, uart.SNN])
            sizes = [len(p.payload) for p in port.calls if p.command == uart.INFER]
            self.assertEqual(sizes, [9600] * 4 + [768] * 4)
            summaries = json.loads((output / 'summary.json').read_text())
            self.assertEqual([s['total'] for s in summaries], [4, 4])
            self.assertEqual([s['accuracy_percent'] for s in summaries], [25.0, 25.0])
            self.assertEqual(summaries[0]['classes'], ['Circle', 'Square', 'Triangle', 'Figure_Eight'])
            self.assertEqual(summaries[1]['classes'], list(sender.CLASSES))
            import csv
            with (output / 'cnn.csv').open() as stream:
                cnn_rows = list(csv.DictReader(stream))
            with (output / 'snn.csv').open() as stream:
                snn_rows = list(csv.DictReader(stream))
            self.assertEqual([r['true_label'] for r in cnn_rows], ['0', '3', '1', '2'])
            self.assertEqual([r['true_label'] for r in snn_rows], ['0', '1', '2', '3'])
            self.assertEqual([r['predicted_class'] for r in cnn_rows], ['Triangle'] * 4)
            self.assertEqual([r['predicted_class'] for r in snn_rows], ['Square'] * 4)
            self.assertTrue((output / 'cnn.csv').is_file() and (output / 'snn.csv').is_file())
            self.assertEqual(len(json.loads((output / 'images.json').read_text())), 4)

    def test_missing_classes_are_not_silently_skipped(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaises(ValueError): sender.select_images(Path(temp), 1)

    def test_existing_reports_are_not_mixed_with_new_run(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp)
            (output / 'summary.json').write_text('old successful report')
            with patch.object(sender, 'select_images', return_value=[(Path('image.png'), 0)]):
                with self.assertRaisesRegex(ValueError, 'not empty'):
                    sender.main(['--model', 'both', '--output', str(output)])


class PreprocessingChecks(unittest.TestCase):
    def test_quantization_endpoints(self):
        with tempfile.TemporaryDirectory() as temp:
            for pixel, expected in [(0, -512), (128, 2), (255, 512)]:
                path = Path(temp) / 'image.png'
                cv2.imwrite(str(path), np.full((60, 80), pixel, dtype=np.uint8))
                result = np.frombuffer(prep.prepare_image(path, 'cnn'), dtype='<i2')
                self.assertEqual(len(result), 4800)
                self.assertTrue(np.all(result == expected))

    def test_preprocessing_matches_original_senders(self):
        old_root = Path.home() / 'Projects/embeded/ai'
        references = [('snn', old_root / 'SNN_guest_lecture/code/send_images_uart_snn.py', 'extract_features'),
                      ('cnn', old_root / 'CNN_guest_lecture/code/send_images_uart.py', 'image_to_q8_8_60x80')]
        for model, reference, function in references:
            if not reference.is_file(): self.skipTest('Original external sender unavailable on this computer')
            spec = importlib.util.spec_from_file_location('reference_' + model, reference)
            module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
            with tempfile.TemporaryDirectory() as temp:
                for shape in ((60, 80), (37, 91), (120, 160)):
                    path = Path(temp) / 'image.png'
                    image = np.random.default_rng(42).integers(0, 256, size=shape, dtype=np.uint8)
                    cv2.imwrite(str(path), image)
                    expected = getattr(module, function)(path).astype('<i2').tobytes()
                    self.assertEqual(prep.prepare_image(path, model), expected)
            # Also compare actual assignment images, not only synthetic inputs.
            if sender.DEFAULT_DATASET.is_dir():
                for path, _ in sender.select_images(sender.DEFAULT_DATASET, 2):
                    expected = getattr(module, function)(path).astype('<i2').tobytes()
                    self.assertEqual(prep.prepare_image(path, model), expected)


if __name__ == '__main__': unittest.main(verbosity=2)
