"""Assignment 2 UART protocol. No image/serial dependencies; see RUN_SWITCHING.md."""
import binascii
import struct
import time
from dataclasses import dataclass

VERSION = 1
FIRMWARE_REVISION = 2
REQUEST_MAGIC = b'\xaa\x55'
RESPONSE_MAGIC = b'\xa5\x5a'
HEADER = struct.Struct('<BBBBHH')  # version, command, model, status, sequence, length
RESULT = struct.Struct('<BI4i')
NONE, CNN, SNN = 0, 1, 2
STATUS, SELECT, INFER, INFO, ECHO = 1, 2, 3, 4, 5
UNSELECTED, READY, FAILED = 0, 1, 2
MODELS = {'cnn': CNN, 'snn': SNN}
INPUT_BYTES = {CNN: 9600, SNN: 768}
ERRORS = {1: 'bad command', 2: 'bad model', 3: 'bad payload length',
          4: 'checksum mismatch', 5: 'FPGA timeout', 6: 'model not ready',
          7: 'hardware/PCAP error', 8: 'fault: restart full FPGA and application',
          9: 'protocol version mismatch'}


class ProtocolError(RuntimeError):
    pass


@dataclass(frozen=True)
class Packet:
    command: int
    model: int
    status: int
    sequence: int
    payload: bytes


def crc16(data):
    return binascii.crc_hqx(data, 0xffff)  # CRC-16/CCITT-FALSE


def encode_packet(command, model, sequence, payload=b'', status=0, response=False):
    payload = bytes(payload)
    if len(payload) > 9600:
        raise ValueError('Payload exceeds 9600 bytes')
    body = HEADER.pack(VERSION, command, model, status, sequence, len(payload)) + payload
    return (RESPONSE_MAGIC if response else REQUEST_MAGIC) + body + struct.pack('<H', crc16(body))


def decode_packet(data, response=False):
    magic = RESPONSE_MAGIC if response else REQUEST_MAGIC
    if len(data) < 12 or data[:2] != magic:
        raise ProtocolError('Bad packet framing')
    version, command, model, status, sequence, length = HEADER.unpack(data[2:10])
    if length > 9600 or len(data) != length + 12:
        raise ProtocolError('Bad packet length')
    if crc16(data[2:-2]) != struct.unpack('<H', data[-2:])[0]:
        raise ProtocolError('Packet checksum mismatch; no automatic retry')
    if version != VERSION:
        raise ProtocolError(f'Unsupported protocol version {version}')
    return Packet(command, model, status, sequence, data[10:-2])


class Client:
    def __init__(self, serial_port, timeout=10.0):
        self.serial = serial_port
        self.timeout = timeout
        self.sequence = 0

    def _read_exact(self, count, deadline):
        data = bytearray()
        while len(data) < count:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise ProtocolError('UART reply timeout; check the new application, bitstream and baud rate')
            self.serial.timeout = min(remaining, 0.2)
            data.extend(self.serial.read(count - len(data)))
        return bytes(data)

    def request(self, command, model=NONE, payload=b''):
        self.sequence = (self.sequence + 1) & 0xffff
        packet = encode_packet(command, model, self.sequence, payload)
        written = self.serial.write(packet)
        if written != len(packet):
            raise ProtocolError('Incomplete UART write; no automatic retry')
        self.serial.flush()
        deadline = time.monotonic() + (max(15.0, self.timeout) if command == SELECT else self.timeout)
        # Skip startup noise while preserving overlapping magic prefixes.
        previous = b''
        while True:
            byte = self._read_exact(1, deadline)
            if previous + byte == RESPONSE_MAGIC:
                break
            previous = byte
        header = self._read_exact(HEADER.size, deadline)
        length = HEADER.unpack(header)[-1]
        if length > (9600 if command == ECHO else RESULT.size):
            raise ProtocolError(f'Invalid reply payload length {length}')
        reply = decode_packet(RESPONSE_MAGIC + header + self._read_exact(length + 2, deadline), response=True)
        if reply.sequence != self.sequence or reply.command != command:
            raise ProtocolError('Unexpected/stale reply; no automatic retry')
        if command != STATUS and reply.model != model:
            raise ProtocolError('Reply model does not match request')
        if reply.status:
            raise ProtocolError(f'FPGA error {reply.status}: {ERRORS.get(reply.status, "unknown error")}')
        return reply

    def status(self):
        reply = self.request(STATUS)
        if len(reply.payload) != 1 or reply.payload[0] not in (UNSELECTED, READY, FAILED):
            raise ProtocolError('Malformed STATUS reply')
        state = reply.payload[0]
        if reply.model not in (NONE, CNN, SNN) or (state == READY) != (reply.model != NONE):
            raise ProtocolError('Inconsistent STATUS model/state')
        return state, reply.model

    def timer_info(self):
        try:
            reply = self.request(INFO)
        except ProtocolError as error:
            raise ProtocolError('Cannot verify the updated C firmware. Stop the old Vitis debug session '
                                'and relaunch hello_world_app_hw_1 to download hello_world/build/hello_world.elf. '
                                f'Detail: {error}') from error
        if len(reply.payload) != 12:
            raise ProtocolError('Malformed firmware/timer INFO reply')
        revision, frequency, ticks = struct.unpack('<III', reply.payload)
        if revision != FIRMWARE_REVISION:
            raise ProtocolError(f'FPGA runs firmware revision {revision}; expected {FIRMWARE_REVISION}. '
                                'Relaunch the Vitis debug configuration to download the rebuilt ELF.')
        if not frequency or not ticks:
            raise ProtocolError('The ARM timer is not advancing. No valid inference timing is available; '
                                'stop and relaunch the Vitis debug configuration.')
        return revision, frequency, ticks

    def select(self, model):
        reply = self.request(SELECT, model)
        if len(reply.payload) != 4:
            raise ProtocolError('Malformed SELECT reply')
        return struct.unpack('<I', reply.payload)[0]

    def echo(self, payload):
        """UART-only round trip: the ARM returns the payload unchanged."""
        reply = self.request(ECHO, NONE, payload)
        if reply.payload != bytes(payload):
            raise ProtocolError('ECHO payload mismatch')
        return reply.payload

    def infer(self, model, payload):
        if model not in INPUT_BYTES or len(payload) != INPUT_BYTES[model]:
            raise ValueError('Payload does not match selected model')
        reply = self.request(INFER, model, payload)
        if len(reply.payload) != RESULT.size:
            raise ProtocolError('Malformed INFER reply')
        pred, elapsed_us, *logits = RESULT.unpack(reply.payload)
        if pred >= 4:
            raise ProtocolError(f'Invalid prediction {pred}')
        return pred, elapsed_us, tuple(logits)
