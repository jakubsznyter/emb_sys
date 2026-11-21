#!/usr/bin/env python3
"""Largest |activation| per layer on the 800 board inputs (from the Q8.8 files),
compared with ap_fixed<16,8> (data_t, +-128) used between layers in ds_cnn.cpp.
Usage: python activation_ranges.py run_norm hls_cnn/inputs_norm.bin
"""
import sys
from pathlib import Path

import numpy as np
import tensorflow as tf

model = tf.keras.models.load_model(Path(sys.argv[1]) / "model.keras")
x = np.fromfile(sys.argv[2], dtype="<i2").astype(np.float32).reshape(-1, 60, 80, 1) / 256.0
probe = tf.keras.Model(model.inputs, [l.output for l in model.layers])
outs = probe.predict(x, batch_size=200, verbose=0)
for layer, out in zip(model.layers, outs):
    m = float(np.abs(out).max())
    print(f"{layer.name:28s} max|x| = {m:10.3f}  {'OVERFLOWS data_t (>128)' if m >= 128 else ''}")
