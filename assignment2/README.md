# Assignment 2 — Accelerated AI for Vision: CNN and SNN with DFX on PYNQ-Z2

Two neural networks classify four shapes (Circle, Square, Triangle, Figure_Eight) on a
PYNQ-Z2 (`xc7z020clg400-1`). Both are HLS accelerators in the same reconfigurable
partition; the ARM switches between them at runtime with Dynamic Function eXchange (DFX),
without restarting the board. Images are sent from a PC over UART.

Group: Alessandro Rampa, Dominik Capko, Igor Turbo, Kuba Sznyter. Tools: Vivado / Vitis 2026.1.

## Results (board, 800 test images, 200 per class)

| | CNN | SNN |
| --- | --- | --- |
| Accuracy | **63.1 %** | **78.6 %** |
| Inference latency | 1.03 ms | 0.156 ms |
| Model switch (DFX partial reconfiguration) | 33 ms | 33 ms |
| DSP / BRAM (whole design) | 84 / 38.5 | 8 / 15 |
| Programmable-logic power (Vivado estimate) | 0.42 W | 0.29 W |

| Class | CNN | SNN |
| --- | --- | --- |
| Circle | 77.5 % | 78.0 % |
| Square | 23.5 % | 58.5 % |
| Triangle | 60.0 % | 86.0 % |
| Figure_Eight | 91.5 % | 92.0 % |

Raw outputs, confusion matrices and timings are in [`results/`](results):
`kuba_baseline_before_fix` (first working build: CNN 45.9 %, SNN 66.1 %), `kuba_after_snn_fix`,
`kuba_after_cnn_retrain` (final) and `kuba_uart_echo` (UART latency without inference).

## Quick start: run it on the board
Everything needed is in [`run_kit/`](run_kit) (bitstream, ARM program, sender, 800 test
images). No Vivado project or license is needed, only `xsdb` and Python.
```bash
cd run_kit
pip install -r scripts/requirements.txt
xsdb program_board.tcl                       # prints BOARD_PROGRAMMED_AND_RUNNING
python scripts/send_images.py --model both --port /dev/ttyUSB1 --limit 200 --output reports/run_1
```
Windows: `--port COMx` (the second PYNQ-Z2 port). Details in [`run_kit/README.md`](run_kit/README.md).

## System
```
PC (Python)  --UART 4 Mbaud-->  ARM Cortex-A9 (hello_world)  --AXI-->  CNN or SNN accelerator
  preprocessing, labels           protocol, DFX switching (PCAP),        (reconfigurable partition,
  accuracy + timing reports       input BRAM, timer, results              control at 0x44A00000)
```
- **CNN** (`vivado/cnn_hls`): 3×3 conv (16) + BN + ReLU + max-pool → depthwise 3×3 + pointwise 1×1 (8)
  → 6×8 average pool to 5×5 → 1×1 classifier → global average. Input 60×80 grayscale,
  normalised (x/255 − 0.5)/0.25, Q8.8 (`ap_fixed<16,8>`).
- **SNN** (`vivado/snn_ip`): Sobel ON/OFF edge spikes on a 12×16 grid (384 features) → 64 → 32 → 4
  LIF neurons, one time step. The PC computes the features.
- **DFX**: the static design keeps the PS, input BRAM (0x40000000, 16 KB), timing GPIO and two AXI
  shutdown managers that isolate the partition during reconfiguration. The full bitstream starts
  with the CNN; the ARM loads either partial bitstream through PCAP on a SELECT command.
  See [`vivado/docs/DFX_SWITCHING.md`](vivado/docs/DFX_SWITCHING.md).

## Accuracy improvements
- **SNN, 66.1 % → 78.6 %:** the first layer shifted every product right by 7 bits before adding it,
  losing the fractions. It now sums the full products and rounds once
  (`vivado/snn_ip/src/snn_mlp.cpp`, `fc1_lif`). C simulation on all 12,000 test images:
  65.25 % → 78.48 %; same latency (15,604 cycles) and resources.
- **CNN, 45.9 % → 63.1 %:** the board already matched the trained model exactly; the model was
  undertrained (10 epochs). The same network and preprocessing were trained to 80 epochs
  ([`cnn_retrain/`](cnn_retrain)), 66.9 % in float. Only the weights changed in hardware. A
  bit-accurate C model of the HLS code (`cnn_retrain/harness.cpp`) predicted 63.1 % on the 800
  board images, and the board matched it on 800/800 predictions.
- Not deployed: normalising each image's polarity and contrast lifts the same CNN to 96 % in float,
  but its weights need 24-bit precision in the HLS design.

## Repository
| Path | Contents |
| --- | --- |
| `run_kit/` | Ready-to-run files and instructions |
| `results/` | Board runs (CSV per image, summaries, confusion matrices) |
| `vivado/assignment2.xpr`, `assignment2.srcs/` | Vivado DFX project (block designs `main`, `CNN`, `SNN`) |
| `vivado/cnn_hls/`, `vivado/ip_repo/cnn/` | CNN HLS source + weights, packaged IP |
| `vivado/snn_ip/` | SNN HLS source, testbench, weights, packaged IP |
| `vivado/hello_world/src/` | ARM application (protocol, DFX runtime) |
| `vivado/bitstreams/` | Full bitstream and both partial BIN images of the final build |
| `vivado/scripts/` | Sender, path setup, build and conversion scripts |
| `vivado/tests/` | Host tests of the protocol and runtime |
| `cnn_retrain/` | CNN training, weight export and bit-accurate checks |

## Rebuilding
Generated Vivado/Vitis folders are not included; the tools recreate them.
1. `cd vivado && python3 scripts/setup_paths.py --tools-root /path/to/2026.1` (rebases paths).
2. Vivado: `vivado -source scripts/open_project.tcl`, then run implementation for both DFX
   configurations (`scripts/rebuild_dfx.tcl`), and `source scripts/partial_to_bin.tcl`.
3. After an HLS change: synthesise and package the IP (`v++ -c --mode hls --config hls_config.cfg`,
   `vitis-run --mode hls --package ...`), upgrade it in the block design, then step 2.
4. ARM application: `vitis -s scripts/update_platform.py` (creates the platform from
   `artifacts/dfx/main_wrapper.xsa`), generate `hello_world/src/cnn.h` and `snn.h` with
   `python3 scripts/bin_to_header.py cnn_bin bitstreams/cnn.bin hello_world/src/cnn.h` (same for
   `snn_bin`), then `bash scripts/build_app.sh`.
5. Program with `xsdb scripts/program_board.tcl` (uses the built `hello_world.elf`).

The dataset (Circle, Square, Triangle from Mendeley Data; Figure_Eight generated, 10,000 per class,
70/30 split) is in the repository root as `split_assignment_dataset.zip`.
