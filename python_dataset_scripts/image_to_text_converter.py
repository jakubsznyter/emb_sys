from pathlib import Path
import cv2
import numpy as np

# 1. Set paths: INPUT_DIR must contain one subfolder per class
INPUT_DIR = Path(r"C:\Vivado_Materials\python\split_assignment_dataset\test")
OUTPUT_DIR = Path(r"C:\Vivado_Materials\python\dwpw_cnn_testing_data_aug_txt")

# Supported image extensions
IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".tif", ".tiff"}

# Network input dimensions (Width x Height for cv2.resize -> 60 rows x 80 cols)
TARGET_WIDTH = 80
TARGET_HEIGHT = 60


def convert_dataset(input_root: Path, output_root: Path) -> None:
    if not input_root.exists():
        raise FileNotFoundError(f"Input directory not found: {input_root}")

    total_converted = 0

    for class_dir in sorted(p for p in input_root.iterdir() if p.is_dir()):
        out_class_dir = output_root / class_dir.name
        out_class_dir.mkdir(parents=True, exist_ok=True)

        class_count = 0
        for img_path in sorted(class_dir.iterdir()):
            if img_path.suffix.lower() not in IMAGE_EXTS:
                continue

            # Load as grayscale uint8 (0-255)
            gray = cv2.imread(str(img_path), cv2.IMREAD_GRAYSCALE)
            if gray is None:
                print(f"[WARN] Could not read image: {img_path}")
                continue

            # Resize to 60 rows x 80 columns matching train_cnn.py
            if gray.shape != (TARGET_HEIGHT, TARGET_WIDTH):
                gray = cv2.resize(
                    gray,
                    (TARGET_WIDTH, TARGET_HEIGHT),
                    interpolation=cv2.INTER_LINEAR,
                )

            # Save as 60x80 space-separated integers (0-255)
            txt_path = out_class_dir / f"{img_path.stem}.txt"
            np.savetxt(txt_path, gray, fmt="%d", delimiter=" ")
            class_count += 1

        print(f"[OK] {class_dir.name}: converted {class_count} images")
        total_converted += class_count

    print(f"[DONE] Total files written to {output_root}: {total_converted}")


if __name__ == "__main__":
    convert_dataset(INPUT_DIR, OUTPUT_DIR)