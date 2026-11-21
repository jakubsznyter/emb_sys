# Assignment 2 — CNN / SNN with DFX on PYNQ-Z2: run kit

Runs both neural-network accelerators on the board and switches between them at
runtime with Dynamic Function eXchange (DFX). No Vivado project or license needed.

| Model | Accuracy (board, 800 images) | Inference | Switch (DFX) |
| --- | --- | --- | --- |
| CNN (retrained, 80 epochs) | 63.12 % | 1.03 ms | 33 ms |
| SNN (rounding fix) | 78.62 % | 0.156 ms | 33 ms |

## What you need
- PYNQ-Z2 connected over micro-USB, powered on, boot jumper on **JTAG**.
- `xsdb` from AMD Vivado/Vitis 2026.1 (or the free Vivado Lab Edition). On Linux run
  `source <install>/Vitis/settings64.sh` first; on Windows use the "Vitis/Vivado command prompt".
- Python 3 with `pip install -r scripts/requirements.txt` (numpy, opencv-python, pyserial).
- Linux: your user in the `dialout` group. Close any serial terminal (Vitis, PuTTY) on the port.

## Run
```bash
# 1. Program the board (about 10 s). Prints BOARD_PROGRAMMED_AND_RUNNING.
xsdb program_board.tcl

# 2. Test both models on the bundled images (about 2 min)
python scripts/send_images.py --model both --port /dev/ttyUSB1 --limit 200 --output reports/run_1
```
- Windows: use the port shown in Device Manager, e.g. `--port COM6`. The PYNQ-Z2 shows two
  ports; the UART is the second one (`/dev/ttyUSB1` on Linux).
- One model only: `--model cnn` or `--model snn`. Fewer images: `--limit 10`.
- Use a new `--output` folder for every run (the script never overwrites one).
- Optional UART latency test (no inference, the ARM echoes each frame):
  `python scripts/uart_echo_test.py --port /dev/ttyUSB1 --limit 25 --output reports/echo_1`

Each run prints every image, then per model: accuracy, confusion matrix, mean inference time
and switch time. It saves `cnn.csv`, `snn.csv`, `summary.json` in the output folder.

## Contents
| Path | What |
| --- | --- |
| `hardware/main_wrapper.bit` | Full bitstream: static design + CNN |
| `hardware/hello_world.elf` | ARM application; contains both partial bitstreams (CNN, SNN) |
| `hardware/ps7_init.tcl` | Zynq PS initialisation |
| `scripts/` | UART sender, protocol, preprocessing, echo test |
| `test_images/` | 200 test images per class (the same 800 as in `results/`) |
| `results/` | Board runs of 21.11: before fixes, after SNN fix, after CNN retrain, UART echo |

## Notes
- Class IDs differ between the models (CNN: Circle, Square, Triangle, Figure_Eight; SNN: Circle,
  Figure_Eight, Square, Triangle). The sender converts labels automatically.
- UART: 4 Mbaud, 8N1, binary protocol (no text output from the board).
- If the sender reports a timeout, reprogram the board with step 1 and run again.
- Full project (Vivado + Vitis + HLS sources): `../vivado`, see the main README.
