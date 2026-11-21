# Assignment 2 — CNN and SNN with DFX on PYNQ-Z2

Two HLS accelerators (CNN and SNN) classify four shapes (Circle, Square, Triangle,
Figure_Eight) on a PYNQ-Z2 (`xc7z020clg400-1`). They share one reconfigurable partition; the
ARM switches between them at runtime with DFX. Images are sent from a PC over UART.
Vivado / Vitis 2026.1.

| Board, 800 test images | CNN | SNN |
| --- | --- | --- |
| Accuracy | 63.1 % | 78.6 % |
| Inference latency | 1.03 ms | 0.156 ms |
| Model switch (DFX) | 33 ms | 33 ms |

## Contents
| Path | What |
| --- | --- |
| `assignment2.xpr`, `assignment2.srcs/` | Vivado DFX project (block designs `main`, `CNN`, `SNN`) |
| `board_files/` | PYNQ-Z2 board definition |
| `cnn_hls/` | CNN HLS source and weights |
| `ip_repo/cnn/` | Packaged CNN IP |
| `snn_ip/` | SNN HLS source, testbench, weights and packaged IP |
| `hello_world/src/` | ARM application: UART protocol, DFX switching through PCAP |
| `artifacts/dfx/main_wrapper.xsa` | Hardware handoff for the Vitis platform |
| `scripts/` | PC sender, path setup and build scripts |

## Build
1. `python3 scripts/setup_paths.py --tools-root /path/to/2026.1`
2. Vivado: `vivado -source scripts/open_project.tcl`, then `source scripts/rebuild_dfx.tcl` (both DFX
   configurations) and `source scripts/partial_to_bin.tcl`.
3. Partial bitstreams into the ARM app:
   `python3 scripts/bin_to_header.py cnn_bin assignment2.runs/impl_1/cnn.bin hello_world/src/cnn.h`
   `python3 scripts/bin_to_header.py snn_bin assignment2.runs/child_0_impl_1/snn.bin hello_world/src/snn.h`
4. Vitis: `vitis -s scripts/update_platform.py`, then `bash scripts/build_app.sh`.
5. Program the full bitstream (`assignment2.runs/impl_1/main_wrapper.bit`) and run `hello_world.elf`.

## Run
```bash
pip install -r scripts/requirements.txt
python scripts/send_images.py --model both --port /dev/ttyUSB1 --dataset <test folder> --limit 200
```
`--model cnn|snn|both`; the test folder needs `Circle`, `Figure_Eight`, `Square`, `Triangle`
subfolders (dataset: `split_assignment_dataset.zip` in the repository root).
