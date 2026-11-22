import os
import random
import shutil
from collections import defaultdict
from pathlib import Path

# --- Configuration ---
SOURCE_DIR = Path("output")       # Folder containing all mixed images
OUTPUT_DIR = Path("selected_dataset")   # Destination folder
NUM_IMAGES = 10000

# Sampling mode:
#   "total_balanced" -> 2,000 images total, split evenly across all classes (recommended)
#   "total_random"   -> 2,000 images total, picked purely at random across all files
#   "per_class"      -> 2,000 images per individual shape class
SAMPLING_MODE = "per_class"

# Optional: restrict to specific prefixes (case-insensitive), or leave None to include all
TARGET_CLASSES = None  # e.g., ["Circle", "Star", "Pentagon", "Square", "Figure_Eight"]

SEED = 42
VALID_EXTS = {".png", ".jpg", ".jpeg", ".bmp"}


def extract_class_name(file_path: Path) -> str:
    """Extract prefix before the trailing '_<id>' (handles multi-word names like 'Figure_Eight')."""
    stem = file_path.stem
    prefix = stem.rsplit("_", 1)[0] if "_" in stem else stem
    # Normalize casing (e.g., 'CIrcle' -> 'Circle', 'figure_eight' -> 'Figure_Eight')
    return "_".join(part.capitalize() for part in prefix.split("_"))


def main() -> None:
    rng = random.Random(SEED)

    if not SOURCE_DIR.is_dir():
        raise FileNotFoundError(f"Source directory not found: {SOURCE_DIR}")

    allowed = {c.lower() for c in TARGET_CLASSES} if TARGET_CLASSES else None

    # Group all valid images by their normalized class prefix
    grouped_files: dict[str, list[Path]] = defaultdict(list)
    for entry in sorted(SOURCE_DIR.iterdir()):
        if entry.is_file() and entry.suffix.lower() in VALID_EXTS:
            cls_name = extract_class_name(entry)
            if allowed is None or cls_name.lower() in allowed:
                grouped_files[cls_name].append(entry)

    if not grouped_files:
        raise RuntimeError(f"No matching images found in {SOURCE_DIR}")

    classes = sorted(grouped_files.keys())
    selected_by_class: dict[str, list[Path]] = defaultdict(list)

    if SAMPLING_MODE == "per_class":
        for cls_name in classes:
            files = grouped_files[cls_name]
            k = min(NUM_IMAGES, len(files))
            if k < NUM_IMAGES:
                print(f"[Warning] '{cls_name}' only has {len(files)} images; selecting all.")
            selected_by_class[cls_name] = rng.sample(files, k)

    elif SAMPLING_MODE == "total_balanced":
        per_class_quota = NUM_IMAGES // len(classes)
        remainder = NUM_IMAGES % len(classes)
        for idx, cls_name in enumerate(classes):
            files = grouped_files[cls_name]
            target = per_class_quota + (1 if idx < remainder else 0)
            k = min(target, len(files))
            if k < target:
                print(f"[Warning] '{cls_name}' only has {len(files)} images (requested {target}).")
            selected_by_class[cls_name] = rng.sample(files, k)

    elif SAMPLING_MODE == "total_random":
        all_files = [(cls_name, f) for cls_name, files in grouped_files.items() for f in files]
        k = min(NUM_IMAGES, len(all_files))
        for cls_name, f in rng.sample(all_files, k):
            selected_by_class[cls_name].append(f)

    else:
        raise ValueError(f"Unknown SAMPLING_MODE: {SAMPLING_MODE}")

    # Copy selected images into class subfolders (non-destructive)
    total_copied = 0
    print(f"Copying images to: {OUTPUT_DIR.resolve()}\n")
    for cls_name in sorted(selected_by_class.keys()):
        dest_folder = OUTPUT_DIR / cls_name
        dest_folder.mkdir(parents=True, exist_ok=True)
        for src_file in selected_by_class[cls_name]:
            shutil.copy2(src_file, dest_folder / src_file.name)
        count = len(selected_by_class[cls_name])
        total_copied += count
        print(f"  {cls_name:<15}: {count} images (out of {len(grouped_files[cls_name])} available)")

    print(f"\nDone. Total selected: {total_copied} images across {len(selected_by_class)} classes.")


if __name__ == "__main__":
    main()