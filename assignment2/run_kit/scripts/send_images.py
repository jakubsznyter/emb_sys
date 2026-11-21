#!/usr/bin/env python3
"""Select CNN/SNN over UART, test the same images, and save separate reports."""
import argparse
import csv
import json
from datetime import datetime
from pathlib import Path
import statistics
import sys
import time

from nn_uart import Client, MODELS, FAILED, ProtocolError

ROOT = Path(__file__).resolve().parents[1]
CLASSES = ('Circle', 'Figure_Eight', 'Square', 'Triangle')
MODEL_CLASSES = {
    'cnn': ('Circle', 'Square', 'Triangle', 'Figure_Eight'),
    'snn': CLASSES,
}
DEFAULT_DATASET = ROOT / 'test_images'  # 200 per class bundled with the kit


def select_images(dataset, limit):
    selected = []
    for label, name in enumerate(CLASSES):
        directory = dataset / name
        if not directory.is_dir():
            raise ValueError(f'Missing class folder: {directory}')
        files = sorted(p for p in directory.iterdir()
                       if p.is_file() and p.suffix.lower() in ('.jpg', '.jpeg', '.png', '.bmp'))
        if not files:
            raise ValueError(f'No images in {directory}')
        selected.extend((path, label) for path in files[:limit] if path.is_file())
    return selected


def test_model(client, model, images, output, prepare):
    model_id = MODELS[model]
    classes = MODEL_CLASSES[model]
    switch_us = client.select(model_id)
    print(f'\n{model.upper()} READY; selection/switch {switch_us / 1000:.3f} ms')
    print('Class IDs: ' + ', '.join(f'{i}={name}' for i, name in enumerate(classes)))
    confusion = [[0] * 4 for _ in range(4)]
    times = []
    csv_path = output / f'{model}.csv'
    # Preserve completed rows if the connection fails partway through a run.
    with csv_path.open('w', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(('model', 'image', 'true_label', 'prediction', 'true_class', 'predicted_class', 'correct',
                         'inference_us', 'roundtrip_us', 'logit0', 'logit1', 'logit2', 'logit3'))
        for path, dataset_label in images:
            true_class = CLASSES[dataset_label]
            label = classes.index(true_class)
            payload = prepare(path, model)
            start = time.perf_counter_ns()
            pred, inference_us, logits = client.infer(model_id, payload)
            roundtrip_us = (time.perf_counter_ns() - start) // 1000
            confusion[label][pred] += 1
            times.append(inference_us)
            writer.writerow((model, str(path), label, pred, true_class, classes[pred], int(pred == label),
                             inference_us, roundtrip_us, *logits))
            stream.flush()
            print(f'{model} {path.name}: true={true_class} pred={classes[pred]} '
                  f'{inference_us} us {"OK" if pred == label else "WRONG"}')
    correct = sum(confusion[i][i] for i in range(4))
    summary = {'model': model, 'total': len(images), 'correct': correct,
               'accuracy_percent': correct * 100.0 / len(images), 'switch_us': switch_us,
               'mean_inference_us': statistics.mean(times), 'min_inference_us': min(times),
               'max_inference_us': max(times), 'confusion_matrix': confusion,
               'classes': list(classes), 'logits': 'raw integers in classes order; scales differ between models'}
    (output / f'{model}_summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(f'{model.upper()}: {correct}/{len(images)} = {summary["accuracy_percent"]:.2f}% '
          f'; mean inference {summary["mean_inference_us"]:.1f} us')
    print('Confusion matrix (rows=true, columns=predicted):')
    for row in confusion:
        print(row)
    return summary


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', choices=('cnn', 'snn', 'both'), required=True)
    parser.add_argument('--dataset', type=Path, default=DEFAULT_DATASET)
    parser.add_argument('--port', default='/dev/ttyUSB1')
    parser.add_argument('--baud', type=int, default=4000000)
    parser.add_argument('--limit', type=int, default=100, help='Maximum images per class')
    parser.add_argument('--output', type=Path, default=None)
    args = parser.parse_args(argv)
    if args.limit < 1:
        parser.error('--limit must be positive')
    images = select_images(args.dataset, args.limit)
    output = args.output or ROOT / 'reports' / ('uart_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
    if output.exists() and any(output.iterdir()):
        raise ValueError(f'Report directory is not empty: {output}; choose a new --output directory')
    output.mkdir(parents=True, exist_ok=True)
    import serial
    from image_preprocessing import prepare_image
    # Store the exact list once so both runs are directly comparable.
    (output / 'images.json').write_text(json.dumps(
        [{'path': str(p), 'label': label, 'class_name': CLASSES[label]} for p, label in images], indent=2) + '\n')
    print(f'{len(images)} images; reports: {output}')
    with serial.Serial(args.port, args.baud, timeout=0.2, write_timeout=5,
                       xonxoff=False, rtscts=False, dsrdtr=False) as port:
        time.sleep(0.2)
        port.reset_input_buffer()
        client = Client(port)
        revision, frequency, ticks = client.timer_info()
        print(f'ARM firmware {revision}; timer {frequency} ticks/s; counter advanced {ticks} ticks')
        state, _ = client.status()
        if state == FAILED:
            raise ProtocolError('FPGA is in FAULT. Reprogram the matching full BIT and restart the application.')
        models = ('cnn', 'snn') if args.model == 'both' else (args.model,)
        summaries = []
        for model in models:
            summaries.append(test_model(client, model, images, output, prepare_image))
        (output / 'summary.json').write_text(json.dumps(summaries, indent=2) + '\n')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (RuntimeError, ValueError, OSError) as error:
        print(f'ERROR: {error}', file=sys.stderr)
        raise SystemExit(1)
