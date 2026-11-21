#!/usr/bin/env python3
"""Continue run_raw (original preprocessing) from epoch 40 to 80 with the same
settings as train_experiment.py; saves to run_raw80/.
Usage: python continue_training.py
"""
import json
import time
from pathlib import Path

import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split
from sklearn.utils import shuffle

import train_experiment as te

HERE = Path(__file__).resolve().parent
src, out = HERE / "run_raw", HERE / "run_raw80"
out.mkdir(exist_ok=True)
prev = json.loads((src / "result.json").read_text())
start_epoch, last_lr = prev["epochs_run"], float(prev["history"]["learning_rate"][-1]) if "learning_rate" in prev["history"] else 7.5e-5

tf.random.set_seed(te.SEED); np.random.seed(te.SEED)
xtr_u8, ytr, xte_u8, yte, board = te.load_all()
x, y = shuffle(te.to_input(xtr_u8, "raw"), ytr, random_state=te.SEED)
x_train, x_val, y_train, y_val = train_test_split(x, y, test_size=te.VAL_SPLIT, stratify=y,
                                                  random_state=te.SEED, shuffle=True)  # same split as before
x_test = te.to_input(xte_u8, "raw")

model = tf.keras.models.load_model(src / "model.keras")
model.optimizer.learning_rate.assign(last_lr)
print(f"[raw80] resuming at epoch {start_epoch}, lr {last_lr:.1e}; start val {te.score(model, x_val, y_val)['accuracy']:.4f}", flush=True)


class Progress(tf.keras.callbacks.Callback):
    def on_epoch_end(self, epoch, logs=None):
        print(f"[raw80] epoch {epoch + 1}: train {logs['accuracy']:.4f} val {logs['val_accuracy']:.4f} "
              f"lr {float(self.model.optimizer.learning_rate.numpy()):.1e}", flush=True)


t0 = time.time()
hist = model.fit(x_train, y_train, validation_data=(x_val, y_val), initial_epoch=start_epoch, epochs=80,
                 batch_size=te.BATCH, verbose=0, callbacks=[
                     tf.keras.callbacks.EarlyStopping(monitor="val_loss", patience=5, restore_best_weights=True),
                     tf.keras.callbacks.ReduceLROnPlateau(monitor="val_loss", factor=0.5, patience=2, min_lr=1e-6),
                     Progress()])
result = {"mode": "raw", "epochs_run": start_epoch + len(hist.history["loss"]), "train_seconds": round(time.time() - t0),
          "val": te.score(model, x_val, y_val), "test_12000": te.score(model, x_test, yte),
          "board_800": te.score(model, x_test[board], yte[board]), "history": hist.history}
model.save(out / "model.keras")
(out / "result.json").write_text(json.dumps(result, indent=2, default=float))
print(f"[raw80] DONE at epoch {result['epochs_run']} ({result['train_seconds']} s): val {result['val']['accuracy']:.4f} "
      f"test {result['test_12000']['accuracy']:.4f} board-800 {result['board_800']['accuracy']:.4f}", flush=True)
print(f"[raw80] board-800 per class: {result['board_800']['per_class']}", flush=True)
