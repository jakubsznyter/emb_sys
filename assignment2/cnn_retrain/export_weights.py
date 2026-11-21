#!/usr/bin/env python3
"""Export a trained run in the lecture HLS weight format (BN-folded, as train_cnn.py).
Usage: python export_weights.py run_norm hls_cnn/weights_norm [pw_scale]
"""
import sys
from pathlib import Path

import numpy as np
import tensorflow as tf

run, out = Path(sys.argv[1]), Path(sys.argv[2]) / "weights_cstyle_folded"
out.mkdir(parents=True, exist_ok=True)
model = tf.keras.models.load_model(run / "model.keras")


def fold(bn_name):
    bn = model.get_layer(bn_name)
    gamma, beta, mean, var = bn.get_weights()
    scale = gamma / np.sqrt(var + bn.epsilon)
    return scale, beta - mean * scale


def c2d(a):
    return "{" + ",\n".join("{" + ", ".join(f"{float(v)}" for v in row) + "}" for row in a) + "}"


def c1d(a):
    return "{" + ", ".join(f"{float(v)}" for v in a) + "}"


s, b = fold("bn1")
w1 = model.get_layer("conv1").get_weights()[0] * s.reshape(1, 1, 1, -1)      # (3,3,1,16)
s2, b2 = fold("bn_dw")
wd = model.get_layer("dw3x3").get_weights()[0] * s2.reshape(1, 1, -1, 1)     # (3,3,16,1)
s3, b3 = fold("bn_pw")
wp = model.get_layer("pw1x1").get_weights()[0] * s3.reshape(1, 1, 1, -1)     # (1,1,16,8)
wc, bc = model.get_layer("cls_1x1").get_weights()                             # (1,1,8,4)

# Optional fixed-point headroom (argv[3], default 1): ReLU and average pooling are
# positively homogeneous, so dividing the pointwise layer (weights + bias) by k and
# multiplying the classifier weights by k gives identical float outputs, while the
# pointwise activations (stored as ap_fixed<16,8>, limit +-128) shrink by k.
k = float(sys.argv[3]) if len(sys.argv) > 3 else 1.0
wp, b3, wc = wp / k, b3 / k, wc * k

files = {
    "layer_1_conv1_WEIGHTS.txt": c2d(w1.reshape(9, 16)), "layer_1_conv1_BIAS.txt": c1d(b),
    "layer_2_dw3x3_WEIGHTS.txt": c2d(np.stack([wd[:, :, c, 0].reshape(9) for c in range(16)])),
    "layer_2_dw3x3_BIAS.txt": c1d(b2),
    "layer_3_pw1x1_WEIGHTS.txt": c2d(wp.reshape(16, 8)), "layer_3_pw1x1_BIAS.txt": c1d(b3),
    "layer_4_cls_1x1_WEIGHTS.txt": c2d(wc.reshape(8, 4)), "layer_4_cls_1x1_BIAS.txt": c1d(bc),
}
for name, text in files.items():
    (out / name).write_text(text)
ranges = {k: (float(np.min(v)), float(np.max(v))) for k, v in
          {"conv1": w1, "b1": b, "dw": wd, "b_dw": b2, "pw": wp, "b_pw": b3, "cls": wc, "b_cls": bc}.items()}
print("exported to", out, "\nvalue ranges (ap_fixed<16,8> holds -128..127.996):", ranges)
