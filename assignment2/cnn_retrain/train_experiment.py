#!/usr/bin/env python3
"""CNN retraining experiment (Kuba, 21.11.2026).

Same architecture and hyper-parameters as train_cnn.py (the lecture script Igor
reused), so the HLS design would not change -- only the weights. Two input modes:

  raw  : grayscale -> 60x80 bilinear -> x/255            (current deployment)
  norm : as raw, then |x - background| stretched to 0..255 per image
         (background = median of the border pixels; shape always bright on dark)

Both then use (x - 0.5) / 0.25 like the board sender. Scored on the full test
folder and on the 800 images used in the board runs (first 200 sorted per class).

Usage: python train_experiment.py raw|norm [max_epochs]
"""
import json
import os
import sys
import time
from pathlib import Path

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
import cv2
import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split
from sklearn.utils import shuffle

HERE = Path(__file__).resolve().parent
DATA = HERE.parent / "split_assignment_dataset"
CACHE = HERE / "cache_60x80_u8.npz"
CLASSES = ["Circle", "Square", "Triangle", "Figure_Eight"]  # board CNN class IDs
IMG_H, IMG_W = 60, 80
SEED = 42
VAL_SPLIT, BATCH, LR = 0.30, 16, 3e-4  # as train_cnn.py
BOARD_PER_CLASS = 200


def load_split(split):
    xs, ys, board = [], [], []
    for label, name in enumerate(CLASSES):
        files = sorted(p for p in (DATA / split / name).iterdir() if p.suffix.lower() == ".png")
        for i, p in enumerate(files):
            img = cv2.imread(str(p), cv2.IMREAD_GRAYSCALE)
            xs.append(cv2.resize(img, (IMG_W, IMG_H), interpolation=cv2.INTER_LINEAR))
            ys.append(label)
            board.append(split == "test" and i < BOARD_PER_CLASS)
    return np.stack(xs), np.array(ys), np.array(board)


def load_all():
    if CACHE.exists():
        d = np.load(CACHE)
        return d["xtr"], d["ytr"], d["xte"], d["yte"], d["board"]
    xtr, ytr, _ = load_split("train")
    xte, yte, board = load_split("test")
    np.savez(CACHE, xtr=xtr, ytr=ytr, xte=xte, yte=yte, board=board)
    return xtr, ytr, xte, yte, board


def normalize_polarity(u8):
    """Per image: distance from the border median, stretched to 0..255 (uint8)."""
    x = u8.astype(np.float32)
    border = np.concatenate([x[:, 0, :], x[:, -1, :], x[:, :, 0], x[:, :, -1]], axis=1)
    bg = np.median(border, axis=1)[:, None, None]
    d = np.abs(x - bg)
    peak = d.reshape(len(d), -1).max(axis=1)[:, None, None]
    out = np.where(peak > 0, d * 255.0 / np.maximum(peak, 1e-6), 0.0)
    return np.rint(out).astype(np.uint8)


def to_input(u8, mode):
    if mode == "norm":
        u8 = normalize_polarity(u8)
    x = u8.astype(np.float32) / 255.0
    return ((x - 0.5) / 0.25)[..., None]


def build_model():  # identical to train_cnn.py
    from tensorflow.keras.models import Sequential
    from tensorflow.keras.layers import (Conv2D, DepthwiseConv2D, BatchNormalization, Activation,
                                         MaxPooling2D, AveragePooling2D, GlobalAveragePooling2D, Input)
    m, e = 0.9, 1e-3
    return Sequential([
        Input((IMG_H, IMG_W, 1)),
        Conv2D(16, (3, 3), padding="same", use_bias=False, name="conv1"),
        BatchNormalization(momentum=m, epsilon=e, name="bn1"), Activation("relu"),
        MaxPooling2D((2, 2)),
        DepthwiseConv2D((3, 3), padding="same", use_bias=False, name="dw3x3"),
        BatchNormalization(momentum=m, epsilon=e, name="bn_dw"), Activation("relu"),
        Conv2D(8, (1, 1), padding="same", use_bias=False, name="pw1x1"),
        BatchNormalization(momentum=m, epsilon=e, name="bn_pw"), Activation("relu"),
        AveragePooling2D(pool_size=(6, 8)),
        Conv2D(len(CLASSES), (1, 1), padding="same", use_bias=True, name="cls_1x1"),
        GlobalAveragePooling2D(), Activation("softmax"),
    ])


def score(model, x, y):
    pred = model.predict(x, batch_size=256, verbose=0).argmax(1)
    cm = np.zeros((4, 4), int)
    np.add.at(cm, (y, pred), 1)
    return {"accuracy": float((pred == y).mean()), "confusion": cm.tolist(),
            "per_class": {c: float(cm[i, i] / cm[i].sum()) for i, c in enumerate(CLASSES)}}


def main():
    mode = sys.argv[1]
    max_epochs = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    tf.random.set_seed(SEED); np.random.seed(SEED)
    out = HERE / f"run_{mode}"
    out.mkdir(exist_ok=True)
    xtr_u8, ytr, xte_u8, yte, board = load_all()
    x, y = shuffle(to_input(xtr_u8, mode), ytr, random_state=SEED)
    x_train, x_val, y_train, y_val = train_test_split(x, y, test_size=VAL_SPLIT, stratify=y,
                                                      random_state=SEED, shuffle=True)
    x_test = to_input(xte_u8, mode)
    model = build_model()
    model.compile(optimizer=tf.keras.optimizers.Adam(LR), loss="sparse_categorical_crossentropy",
                  metrics=["accuracy"])

    class Progress(tf.keras.callbacks.Callback):
        def on_epoch_end(self, epoch, logs=None):
            print(f"[{mode}] epoch {epoch + 1}: train {logs['accuracy']:.4f} val {logs['val_accuracy']:.4f} "
                  f"lr {float(self.model.optimizer.learning_rate.numpy()):.1e}", flush=True)

    t0 = time.time()
    hist = model.fit(x_train, y_train, validation_data=(x_val, y_val), epochs=max_epochs, batch_size=BATCH,
                     verbose=0, callbacks=[
                         tf.keras.callbacks.EarlyStopping(monitor="val_loss", patience=5, restore_best_weights=True),
                         tf.keras.callbacks.ReduceLROnPlateau(monitor="val_loss", factor=0.5, patience=2, min_lr=1e-6),
                         Progress()])
    result = {"mode": mode, "epochs_run": len(hist.history["loss"]), "train_seconds": round(time.time() - t0),
              "val": score(model, x_val, y_val), "test_12000": score(model, x_test, yte),
              "board_800": score(model, x_test[board], yte[board]), "history": hist.history}
    model.save(out / "model.keras")
    (out / "result.json").write_text(json.dumps(result, indent=2, default=float))
    print(f"[{mode}] DONE after {result['epochs_run']} epochs ({result['train_seconds']} s): "
          f"val {result['val']['accuracy']:.4f}  test {result['test_12000']['accuracy']:.4f}  "
          f"board-800 {result['board_800']['accuracy']:.4f}", flush=True)
    print(f"[{mode}] board-800 per class: {result['board_800']['per_class']}", flush=True)


if __name__ == "__main__":
    main()
