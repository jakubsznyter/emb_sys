#!/usr/bin/env python3
"""Prepare the bit-accurate CNN check (host python with numpy + cv2).

1. weights_igor/: Igor's weights decoded from the packaged VHDL (cnn_audit), in the
   lecture weight-file format (floats = Q8.8 raw / 256, exactly representable).
2. inputs_raw.bin / inputs_norm.bin: the 800 board images as int16 Q8.8, with the
   board sender's preprocessing (raw) or with the per-image polarity normalisation.
   Order = reports/kuba_after_snn_fix/images.json, so results line up with cnn.csv.
"""
import json
import sys
from pathlib import Path

import cv2
import numpy as np

HERE = Path(__file__).resolve().parent
A2 = HERE.parents[1] / 'assignment2'
sys.path.insert(0, str(A2 / 'cnn_audit'))
import audit_cnn  # noqa: E402  (reads ip_repo/cnn VHDL ROMs)


def c2d(a):
    return '{' + ',\n'.join('{' + ', '.join(repr(float(v)) for v in row) + '}' for row in a) + '}'


def c1d(a):
    return '{' + ', '.join(repr(float(v)) for v in a) + '}'


def write_weights(folder, conv_w, conv_b, dw_w, dw_b, pw_w, pw_b, cls_w, cls_b):
    d = folder / 'weights_cstyle_folded'
    d.mkdir(parents=True, exist_ok=True)
    files = {'layer_1_conv1_WEIGHTS.txt': c2d(conv_w), 'layer_1_conv1_BIAS.txt': c1d(conv_b),
             'layer_2_dw3x3_WEIGHTS.txt': c2d(dw_w), 'layer_2_dw3x3_BIAS.txt': c1d(dw_b),
             'layer_3_pw1x1_WEIGHTS.txt': c2d(pw_w), 'layer_3_pw1x1_BIAS.txt': c1d(pw_b),
             'layer_4_cls_1x1_WEIGHTS.txt': c2d(cls_w), 'layer_4_cls_1x1_BIAS.txt': c1d(cls_b)}
    for name, text in files.items():
        (d / name).write_text(text)


def normalize_polarity(u8):
    x = u8.astype(np.float32)
    border = np.concatenate([x[0, :], x[-1, :], x[:, 0], x[:, -1]])
    d = np.abs(x - np.median(border))
    peak = d.max()
    return np.rint(d * 255.0 / peak).astype(np.uint8) if peak > 0 else np.zeros_like(u8)


def q88(u8):  # identical to scripts/image_preprocessing.py
    px = u8.astype(np.int32).reshape(-1)
    return np.clip((1024 * px + 127) // 255 - 512, -32768, 32767).astype('<i2')


def main():
    conv, conv_b, dw, dw_b, pw, pw_b, cls, cls_b = audit_cnn.weights()
    s = 1 / 256.0
    write_weights(HERE / 'weights_igor',
                  conv.reshape(9, 16) * s, conv_b * s,
                  dw.reshape(9, 16).T * s, dw_b * s,
                  pw * s, pw_b * s, cls * s, cls_b * s)
    images = json.loads((A2 / 'reports/kuba_after_snn_fix/images.json').read_text())
    raw, norm = [], []
    for item in images:
        path = Path(item['path'])
        g = cv2.imread(str(path), cv2.IMREAD_GRAYSCALE)
        g = cv2.resize(g, (80, 60), interpolation=cv2.INTER_LINEAR)
        raw.append(q88(g)); norm.append(q88(normalize_polarity(g)))
    np.concatenate(raw).tofile(HERE / 'inputs_raw.bin')
    np.concatenate(norm).tofile(HERE / 'inputs_norm.bin')
    print('weights shapes', conv.shape, dw.shape, pw.shape, cls.shape, '; images', len(images))


if __name__ == '__main__':
    main()
