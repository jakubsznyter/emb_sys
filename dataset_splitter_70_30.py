import random
import shutil
from collections import defaultdict
from pathlib import Path

# ============================================================
# Configuration
# ============================================================
SOURCE_DIR = Path(r"C:\Vivado_Materials\python\assignment_dataset")
OUTPUT_DIR = Path(r"C:\Vivado_Materials\python\split_assignment_dataset")

TRAIN_RATIO = 0.70          # 70% train, 30% test
MODE = "copy"               # "copy" (safe, keeps originals) or "move" (instant on same drive)
SEED = 42                   # Fixed seed for reproducible splits
VALID_EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".tif", ".tiff"}


def extract_class_from_filename(file_path: Path) -> str:
    """Fallback for flat folders: 'CIrcle_001.png' -> 'Circle', 'Figure_Eight_001.png' -> 'Figure_Eight'."""
    stem = file_path.stem
    prefix = stem.rsplit("_", 1)[0] if "_" in stem else stem
    return "_".join(part.capitalize() for part in prefix.split("_"))


def collect_images_by_class(source_dir: Path) -> dict[str, list[Path]]:
    if not source_dir.is_dir():
        raise FileNotFoundError(f"Source directory not found: {source_dir}")

    grouped: dict[str, list[Path]] = defaultdict(list)

    # Check if images are organized in class subfolders
    subdirs = sorted([d for d in source_dir.iterdir() if d.is_dir() and not d.name.startswith(".")])
    if subdirs:
        for cls_dir in subdirs:
            files = sorted(
                f for f in cls_dir.iterdir()
                if f.is_file() and f.suffix.lower() in VALID_EXTS
            )
            if files:
                grouped[cls_dir.name] = files
    else:
        # Fallback: group flat directory files by filename prefix
        for entry in sorted(source_dir.iterdir()):
            if entry.is_file() and entry.suffix.lower() in VALID_EXTS:
                cls_name = extract_class_from_filename(entry)
                grouped[cls_name].append(entry)

    if not grouped:
        raise RuntimeError(f"No valid images found in {source_dir}")

    return dict(grouped)


def transfer_files(files: list[Path], dest_dir: Path, mode: str) -> None:
    dest_dir.mkdir(parents=True, exist_ok=True)
    transfer_fn = shutil.copy2 if mode == "copy" else shutil.move
    for src in files:
        transfer_fn(src, dest_dir / src.name)


def main() -> None:
    if MODE not in {"copy", "move"}:
        raise ValueError("MODE must be 'copy' or 'move'")

    rng = random.Random(SEED)
    grouped_files = collect_images_by_class(SOURCE_DIR)

    total_train = 0
    total_test = 0

    print(f"Source : {SOURCE_DIR.resolve()}")
    print(f"Output : {OUTPUT_DIR.resolve()} (mode='{MODE}', ratio={TRAIN_RATIO:.0%}/{1 - TRAIN_RATIO:.0%})\n")
    print(f"{'Class':<20} {'Total':>8} {'Train (70%)':>13} {'Test (30%)':>12}")
    print("-" * 57)

    for cls_name in sorted(grouped_files.keys()):
        files = list(grouped_files[cls_name])
        rng.shuffle(files)

        split_idx = int(len(files) * TRAIN_RATIO)
        train_files = files[:split_idx]
        test_files = files[split_idx:]

        transfer_files(train_files, OUTPUT_DIR / "train" / cls_name, MODE)
        transfer_files(test_files, OUTPUT_DIR / "test" / cls_name, MODE)

        total_train += len(train_files)
        total_test += len(test_files)
        print(f"{cls_name:<20} {len(files):>8,} {len(train_files):>13,} {len(test_files):>12,}")

    print("-" * 57)
    print(f"{'TOTAL':<20} {total_train + total_test:>8,} {total_train:>13,} {total_test:>12,}")


if __name__ == "__main__":
    main()