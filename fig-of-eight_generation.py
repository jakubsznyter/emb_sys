import os
import uuid
import cv2
import numpy as np

# Configuration matching the existing 2D Geometric Shapes dataset
OUTPUT_ROOT = "dataset"
CLASS_NAME = "Figure_Eight"
IMG_SIZE = 200  # 200x200 RGB PNGs matching your reference images

# Set to None to save all images into one folder (e.g., dataset/Figure_Eight/)
# or use a dict like {"train": 700, "val": 300, "test": 300} for pre-split folders
SPLITS = None
TOTAL_IMAGES = 10000

rng = np.random.default_rng(42)


def sample_contrasting_colors(min_rgb_dist: float = 60.0) -> tuple[tuple[int, int, int], tuple[int, int, int]]:
    """Sample flat RGB background and foreground colors matching the dataset palette."""
    bg = rng.integers(0, 256, size=3)
    while True:
        fg = rng.integers(0, 256, size=3)
        if np.linalg.norm(fg.astype(np.float32) - bg.astype(np.float32)) >= min_rgb_dist:
            # Convert RGB to BGR for OpenCV
            return (int(bg[2]), int(bg[1]), int(bg[0])), (int(fg[2]), int(fg[1]), int(fg[0]))


def generate_figure_eight_rgb(img_size: int = 200) -> np.ndarray:
    bg_bgr, fg_bgr = sample_contrasting_colors(min_rgb_dist=60.0)
    img = np.full((img_size, img_size, 3), bg_bgr, dtype=np.uint8)

    # 1. Wide size variation (from small shapes like the sample Square to large like the Circle)
    a = rng.uniform(15.0, 82.0)                  # Half-length of the figure-eight
    b = a * rng.uniform(0.75, 1.25)              # Lobe width ratio

    # Proportional loop thickness keeps the two inner holes visible at all scales
    # and matches the visual weight of filled shapes when resized to 60x80
    thickness = max(2, int(round(a * rng.uniform(0.18, 0.28))))

    # 2. Full orientation variation (0 to 360 degrees)
    theta = rng.uniform(0, 2 * np.pi)
    cos_t, sin_t = np.cos(theta), np.sin(theta)

    # Parametric Lemniscate of Gerono
    t = np.linspace(0, 2 * np.pi, 300, endpoint=False)
    x0 = a * np.cos(t)
    y0 = b * np.sin(t) * np.cos(t)

    x_rot = x0 * cos_t - y0 * sin_t
    y_rot = x0 * sin_t + y0 * cos_t

    # 3. Position variation anywhere inside the canvas with a safe margin
    half_thick = thickness / 2.0 + 3.0
    min_x = int(np.ceil(np.max(-x_rot) + half_thick))
    max_x = int(np.floor(img_size - np.max(x_rot) - half_thick))
    min_y = int(np.ceil(np.max(-y_rot) + half_thick))
    max_y = int(np.floor(img_size - np.max(y_rot) - half_thick))

    cx = rng.uniform(min_x, max(min_x + 1, max_x))
    cy = rng.uniform(min_y, max(min_y + 1, max_y))

    pts = np.column_stack((np.round(x_rot + cx), np.round(y_rot + cy))).astype(np.int32)

    # LINE_8 produces crisp, stair-stepped edges with no anti-aliasing or blur
    cv2.polylines(img, [pts], isClosed=True, color=fg_bgr, thickness=thickness, lineType=cv2.LINE_8)
    return img


def save_batch(folder: str, count: int) -> None:
    os.makedirs(folder, exist_ok=True)
    for _ in range(count):
        sample = generate_figure_eight_rgb(IMG_SIZE)
        filename = f"{CLASS_NAME}_{uuid.uuid1()}.png"
        cv2.imwrite(os.path.join(folder, filename), sample)


if SPLITS is None:
    save_batch(os.path.join(OUTPUT_ROOT, CLASS_NAME), TOTAL_IMAGES)
else:
    for split, count in SPLITS.items():
        save_batch(os.path.join(OUTPUT_ROOT, split, CLASS_NAME), count)

print("Generated Figure_Eight dataset in:", os.path.abspath(OUTPUT_ROOT))