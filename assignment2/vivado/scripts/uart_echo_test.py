#!/usr/bin/env python3
"""UART latency test without the accelerator (shared-protocol version of
Alessandro's UART_ECHO_TEST). Sends real preprocessed images, the ARM returns
the same bytes, and the round trip is timed on the PC.

Example:
  python scripts/uart_echo_test.py --port /dev/ttyUSB1 --dataset ~/Desktop/emb_sys2/split_assignment_dataset/test
"""
import argparse
import json
import statistics
import sys
import time
from datetime import datetime
from pathlib import Path

from nn_uart import Client, ProtocolError
from send_images import CLASSES, DEFAULT_DATASET, ROOT, select_images

FRAME_OVERHEAD = 2 + 8 + 2  # magic + header + CRC


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--dataset', type=Path, default=DEFAULT_DATASET)
    parser.add_argument('--port', default='/dev/ttyUSB1')
    parser.add_argument('--baud', type=int, default=4000000)
    parser.add_argument('--limit', type=int, default=10, help='Images per class')
    parser.add_argument('--output', type=Path, default=None)
    args = parser.parse_args(argv)

    import serial
    from image_preprocessing import prepare_image
    images = select_images(args.dataset, args.limit)
    output = args.output or ROOT / 'reports' / ('uart_echo_' + datetime.now().strftime('%Y%m%d_%H%M%S'))
    output.mkdir(parents=True, exist_ok=True)

    results = {}
    with serial.Serial(args.port, args.baud, timeout=0.2, write_timeout=5) as port:
        time.sleep(0.2)
        port.reset_input_buffer()
        client = Client(port)
        for model in ('cnn', 'snn'):
            times_ms = []
            for path, _ in images:
                payload = prepare_image(path, model)
                start = time.perf_counter_ns()
                try:
                    client.echo(payload)
                except ProtocolError as error:
                    if 'bad command' in str(error):
                        raise ProtocolError('This firmware has no ECHO command; rebuild and relaunch hello_world.') from error
                    raise
                times_ms.append((time.perf_counter_ns() - start) / 1e6)
            nbytes = len(payload)
            # 10 bits per byte (start + 8 data + stop), both directions.
            theory_ms = 2 * (nbytes + FRAME_OVERHEAD) * 10 / args.baud * 1000
            results[model] = {'payload_bytes': nbytes, 'images': len(times_ms),
                              'mean_roundtrip_ms': statistics.mean(times_ms),
                              'min_roundtrip_ms': min(times_ms), 'max_roundtrip_ms': max(times_ms),
                              'theoretical_wire_roundtrip_ms': theory_ms}
            r = results[model]
            print(f'{model.upper()} payload {nbytes} B: round trip mean {r["mean_roundtrip_ms"]:.2f} ms '
                  f'(min {r["min_roundtrip_ms"]:.2f}, max {r["max_roundtrip_ms"]:.2f}); '
                  f'wire time alone {theory_ms:.2f} ms')
    (output / 'echo_summary.json').write_text(json.dumps(results, indent=2) + '\n')
    print(f'Saved {output / "echo_summary.json"}')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (RuntimeError, ValueError, OSError) as error:
        print(f'ERROR: {error}', file=sys.stderr)
        raise SystemExit(1)
