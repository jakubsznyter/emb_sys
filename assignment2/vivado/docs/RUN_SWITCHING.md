# Run CNN and SNN with one sender

The existing `hello_world` application now supports both models. No HLS or FPGA
bitstreams were changed for this software update. Its Vitis platform, BSP, launch
BIT and PS initialization now use the matching isolated DFX hardware.

## First run

1. Reopen Vitis using the `assignment2` workspace. Build `platform`, then
   `hello_world` if you have changed anything; both are already built in this copy.
2. Launch the existing `hello_world_app_hw_1` debug configuration. It programs the
   matching full CNN bitstream and starts the ARM application. Let it run rather
   than leaving it paused at `main`.
3. Close Vitis's serial terminal before opening the Python sender. There is no
   ASCII startup banner: UART carries the binary command protocol at **4 Mbaud,
   8 data bits, no parity, one stop bit**, with no flow control.
4. From a terminal in the project directory, use a Python with NumPy, OpenCV and
   pyserial. Your existing environment is ready:

```bash
source /home/dodo/Projects/embeded/ai/.venv/bin/activate
python scripts/send_images.py --model both --port /dev/ttyUSB1 --limit 10
```

This uses your existing assignment test dataset. You can override it:

```bash
python scripts/send_images.py --model cnn --dataset /path/to/test --port /dev/ttyUSB1
python scripts/send_images.py --model snn --dataset /path/to/test --port /dev/ttyUSB1
python scripts/send_images.py --model both --dataset /path/to/test --port /dev/ttyUSB1
```

On another computer, install `scripts/requirements.txt` into a Python virtualenv.
The dataset must contain `Circle`, `Figure_Eight`, `Square`, `Triangle` folders;
the models use different native class IDs:

| ID | CNN | SNN |
| --- | --- | --- |
| 0 | Circle | Circle |
| 1 | Square | Figure_Eight |
| 2 | Triangle | Square |
| 3 | Figure_Eight | Triangle |

The sender converts each image's true class into the selected model's class ID.
Each model's CSV numeric labels, logits and confusion-matrix axes follow its own
`classes` list. CSV files also include `true_class` and `predicted_class` names.
`images.json` keeps the common dataset enumeration and includes `class_name`, so
both runs still use the exact same files even though their class IDs differ.
Missing or empty class folders stop the test instead of quietly reducing it.
The default limit is 100 images per class, selected in filename order.

`both` selects CNN, tests its image list, selects SNN and tests the exact same
list without rebooting the board. Changing `--model` handles selection,
preprocessing and reply decoding automatically. It does not alter the training,
weights or feature calculations. The original standalone sender scripts remain
unchanged and use their old protocols; use the new sender with this application.

Reports go under `reports/uart_<timestamp>` or a directory chosen by `--output`:
`images.json`, `cnn.csv` / `snn.csv`, per-model summary JSON and `summary.json`.
A nonempty `--output` directory is rejected to avoid mixing old and new reports.
Reports include accuracy, confusion matrices, raw logits, inference time,
UART round-trip time and model-selection/switch time. UART/preprocessing/switch
time is excluded from inference time. Logits have model-specific scales and
should not be compared as calibrated confidence scores. Completed CSV rows are
retained if a test fails, but no successful final summary is written for that run.

## Switching and recovery

The application starts UNSELECTED even though the full BIT starts with CNN.
The first SELECT always reloads the requested partial; this also handles an ARM
application restart when the FPGA still holds SNN. A later selection of the
already active model returns READY with zero switching time.

The ARM stops auto-restart and waits for idle when it knows the active model,
requests shutdown on both AXI paths and waits for acknowledgment, copies the
selected BIN unchanged into a 64-byte-aligned DDR buffer, flushes the cache and
loads it through PCAP. It waits for DMA, PCAP and configuration/startup DONE
(`RESET_AFTER_RECONFIG` is enabled), then releases memory followed by control,
checks idle and initializes the input pointer before acknowledging READY.
The shutdown blocks are controlled through their static AXI-Lite registers.

The application explicitly initializes the BSP sleep callback, enables the
Cortex-A9 global counter with prescaler zero, and warms sleep once before any
timing or deadline polling. This also handles a debugger restart with old BSP
initialization flags still in memory. Frequency expressions are parenthesized
when converting ticks to microseconds.

The sender checks firmware revision **2** and verifies that the counter advances
before requesting inference. The first diagnostic line should read
`ARM firmware 2; timer ... ticks/s; counter advanced ... ticks`.
If the running application lacks this INFO command, Python stops with instructions
to stop the old debug session and relaunch `hello_world_app_hw_1`, downloading
`hello_world/build/hello_world.elf`. Rerunning Python alone does not update C code
already running on the processor. A stopped counter also stops the test with an
explicit error rather than silently reporting zero time. Earlier saved zero
measurements cannot be recovered; rerun after loading this executable.

Waits are bounded: 1 second per accelerator/shutdown transition, 5 seconds for
PCAP, and 2 seconds to receive a complete UART frame. Python allows at least
15 seconds for SELECT. A checksum/length/model error rejects that packet before
inference. A switching or accelerator hardware failure enters FAULT, requests
both isolation paths, refuses further work, and reports an error. Python does
not automatically retry timed-out or damaged replies, avoiding accidental
repetition of a request that might already have run.

For FAULT, stop the sender, program the matching full BIT again and restart the
application using the debug launch. For a UART reply timeout, check that the CPU
is running, the serial port is free, and the new application and bitstream were
both loaded. Do not mix partials or launch files from the old hardware build.

## Small binary protocol

Requests begin `AA 55`; replies begin `A5 5A`. All multibyte fields are little-endian.
After magic, the common header is:

| Field | Bytes |
| --- | --- |
| Protocol version (1) | 1 |
| Command: STATUS=1, SELECT_MODEL=2, INFER=3, INFO=4 | 1 |
| Model: NONE=0, CNN=1, SNN=2 | 1 |
| Status: zero in requests; zero means success in replies | 1 |
| Sequence number, echoed in the reply | 2 |
| Payload byte length | 2 |

The payload is followed by a 2-byte CRC-16/CCITT-FALSE (polynomial 0x1021,
initial value 0xFFFF, no reflection/final XOR), covering the header and payload,
excluding magic and the CRC itself. Maximum payload is 9600 bytes.

- INFO: model NONE, empty request. Reply payload is three uint32 values:
  firmware revision (2), timer ticks per second, and counter advancement during
  a bounded probe. This is an additive protocol-v1 command for startup checks.
- STATUS: model NONE, empty request. Reply model is the current active model;
  payload is one byte: UNSELECTED=0, READY=1, FAULT=2. NONE accompanies UNSELECTED/FAULT.
- SELECT_MODEL: model CNN/SNN, empty request. Success means READY; reply payload
  is a 32-bit switching duration in microseconds.
- INFER: selected model plus 9600 CNN bytes (4800 little-endian int16 pixels) or
  768 SNN bytes (384 little-endian int16 features). Success payload is prediction
  (one byte), inference microseconds (uint32), then four raw signed int32 logits:
  21 bytes total. CNN's packed int16 logits are sign-extended by the ARM.
- Errors carry no payload. Codes: 1 bad command, 2 bad model, 3 bad length,
  4 bad CRC, 5 timeout, 6 not ready, 7 hardware/PCAP error, 8 fault, 9 bad version.

Only one request is outstanding at a time. Reconfiguration only occurs on SELECT,
not on individual inference packets. Input preparation stays on the PC: CNN uses
`round(1024*pixel/255 - 512)` as implemented by the supplied integer sender;
SNN uses its original Sobel/ON-OFF regional features scaled by 128.

## Build and checks

In a Vitis 2026.1 environment, `vitis -s scripts/update_platform.py` refreshes and
builds the platform from `artifacts/dfx/main_wrapper.xsa`. `bash scripts/build_app.sh`
builds only the ARM application. Neither command runs FPGA implementation.

```bash
python tests/test_switching.py
python scripts/verify_dfx_package.py
python scripts/verify_preserved.py
python scripts/verify_software_package.py
```

The host tests compile the real runtime with fake MMIO/PCAP, test protocol/CRC
compatibility in C and Python, both-model reporting and preprocessing equality.
The original-sender comparison is skipped on computers without those reference
scripts. It passed here on synthetic inputs and actual assignment images.

The old software configuration is saved in `artifacts/software_before_switching.tar.gz`.
Hardware reports remain in `docs/dfx_upgrade`; software checks are in
`docs/software_switching`. **Board switching and accuracy have not been tested
here because no UART device is connected.**
