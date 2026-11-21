"""Training-matched preprocessing copied from the original working senders."""
from pathlib import Path
import cv2
import numpy as np


IMG_H = 60

IMG_W = 80

T_STEPS = 1

GRID_H = 12

GRID_W = 16

N_FEATURES = GRID_H * GRID_W * 2

SOBEL_PERCENTILE = 90

ALPHA = 3.0

FEATURE_SCALE = 128

def image_to_spikes(img_u8):

    img_u8 = cv2.resize(
        img_u8,
        (IMG_W, IMG_H),
        interpolation=cv2.INTER_LINEAR,
    )

    img = img_u8.astype(np.float32) / 255.0

    sobel_x = cv2.Sobel(img, cv2.CV_32F, 1, 0, ksize=3)
    sobel_y = cv2.Sobel(img, cv2.CV_32F, 0, 1, ksize=3)

    sobel_combined = sobel_x + sobel_y

    pos_edge = np.maximum(sobel_combined, 0)
    neg_edge = np.maximum(-sobel_combined, 0)

    pos_edge /= pos_edge.max() + 1e-8
    neg_edge /= neg_edge.max() + 1e-8

    on_mask = pos_edge > np.percentile(pos_edge, SOBEL_PERCENTILE)
    off_mask = neg_edge > np.percentile(neg_edge, SOBEL_PERCENTILE)

    spikes = np.zeros((T_STEPS, 2, IMG_H, IMG_W), dtype=np.float32)

    if T_STEPS == 1:
        spikes[0, 0, on_mask] = 1.0
        spikes[0, 1, off_mask] = 1.0
    else:
        on_time = (np.exp(-ALPHA * pos_edge) * (T_STEPS - 1)).astype(np.int32)
        off_time = (np.exp(-ALPHA * neg_edge) * (T_STEPS - 1)).astype(np.int32)

        ys, xs = np.where(on_mask)
        spikes[on_time[ys, xs], 0, ys, xs] = 1.0

        ys, xs = np.where(off_mask)
        spikes[off_time[ys, xs], 1, ys, xs] = 1.0

    return spikes

def spikes_to_sparse_regional_features(spikes):

    T, _, H, W = spikes.shape

    cell_h = H // GRID_H
    cell_w = W // GRID_W

    all_features = []

    for t in range(T):
        on_map = spikes[t, 0]
        off_map = spikes[t, 1]

        features = []

        for gy in range(GRID_H):
            y0 = gy * cell_h
            y1 = H if gy == GRID_H - 1 else (gy + 1) * cell_h

            for gx in range(GRID_W):
                x0 = gx * cell_w
                x1 = W if gx == GRID_W - 1 else (gx + 1) * cell_w

                on_cell = on_map[y0:y1, x0:x1]
                off_cell = off_map[y0:y1, x0:x1]

                area = float((y1 - y0) * (x1 - x0))

                features.append(float(on_cell.sum()) / area)
                features.append(float(off_cell.sum()) / area)

        all_features.append(features)

    return np.array(all_features, dtype=np.float32)

def extract_features(image_path: Path) -> np.ndarray:

    img = cv2.imread(str(image_path), cv2.IMREAD_GRAYSCALE)

    if img is None:
        raise RuntimeError(f"Could not read image: {image_path}")

    spikes = image_to_spikes(img)
    features_float = spikes_to_sparse_regional_features(spikes).reshape(-1)

    features_int16 = np.round(features_float * FEATURE_SCALE).astype(np.int16)

    assert features_int16.shape[0] == T_STEPS * N_FEATURES

    return features_int16

N_PIXELS = IMG_H * IMG_W

def image_to_q8_8_60x80(image_path: Path) -> np.ndarray:
    img = cv2.imread(str(image_path), cv2.IMREAD_GRAYSCALE)

    if img is None:
        raise RuntimeError(f"Could not read image: {image_path}")

    if img.shape != (IMG_H, IMG_W):
        img = cv2.resize(img, (IMG_W, IMG_H), interpolation=cv2.INTER_LINEAR)

    img_u8 = img.astype(np.uint8).reshape(-1)

    px = img_u8.astype(np.int32)
    q = ((1024 * px + 127) // 255 - 512).astype(np.int32)
    q = np.clip(q, -32768, 32767).astype(np.int16)

    assert q.shape[0] == N_PIXELS

    return q

def prepare_image(path, model):
    if model not in ('cnn', 'snn'):
        raise ValueError(f'Unknown model: {model}')
    values = image_to_q8_8_60x80(path) if model == "cnn" else extract_features(path)
    return values.astype("<i2", copy=False).tobytes(order="C")

