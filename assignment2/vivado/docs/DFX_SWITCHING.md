# CNN/SNN DFX hardware and runtime

The shared `hello_world` application now controls both models using the isolated
DFX hardware. Its platform, BSP and launch BIT match the new handoff. See
[RUN_SWITCHING.md](RUN_SWITCHING.md) for commands, protocol, build and recovery.
The pre-existing SNN-source/CNN-platform mismatch was resolved during software
integration; its earlier files are saved in `artifacts/software_before_switching.tar.gz`.

## Connections and addresses

```text
CPU -> SmartConnect -> nn_control_shutdown -> CNN_0/s_axi_CTRL_0
CNN_0/m_axi_gmem_0 -> nn_memory_shutdown -> SmartConnect -> input BRAM

CPU -> SmartConnect -> each shutdown manager's separate S_AXI_CTRL interface
```

Both managers stay outside the reconfigurable Pblock. They use the existing
100 MHz FCLK and active-low static reset. The control manager is configured for
an RP slave, AXI4-Lite, 32-bit data/address. The memory manager is configured for
an RP master, AXI4, 32-bit data, 64-bit address, and one AXI ID bit. Its interface
parameters are explicitly configured; Vivado's automatic endpoint-inference
warning on this DFX path does not mean the widths are left unspecified.

| Resource | CPU base | Address window |
| --- | --- | --- |
| Input BRAM | `0x40000000` | 16 KB; 4096 x 32-bit words |
| Existing timing GPIO | `0x40010000` | 64 KB |
| Control shutdown manager | `0x40020000` | 64 KB |
| Memory shutdown manager | `0x40030000` | 64 KB |
| Active CNN/SNN control | `0x44A00000` | 64 KB |

Both variants' memory apertures and CPU/accelerator BRAM assignments cover 16 KB.
The accelerator memory master is excluded from GPIO, its own control registers,
and both shutdown-manager control windows. The CPU can access all five targets.

`hello_world/src/dfx_hw.h` defines these constants for the shared application.
Compile-time checks reject a BSP with incompatible addresses or BRAM size. The new DFX static hardware must
be used with its matching new partial bitstreams, not the previous build's pair.

## Shutdown-manager register interface

Both managers use offset **0x00** for CONTROL on writes and STATUS on reads:

- Write bit 0 = 1 to request shutdown; write zero to request pass-through.
- Read bit 0 = shutdown requested.
- Read bit 1 = both AXI channels are in shutdown.
- Read bit 2 = write channel is in shutdown; bit 3 = read channel is in shutdown.

Use finite timeouts while polling. A successful register write alone does not
establish that all previous traffic has completed. The manager's shutdown status
must acknowledge completion before reconfiguration starts.

Reference: [AMD PG377, Register Space and Operation](https://docs.amd.com/v/u/en-US/pg377-dfx-axi-shutdown-manager).

## Implemented C switching sequence

1. Stop accepting inference requests and finish the active inference; disable
   auto-restart. Confirm the accelerator is idle before closing its control path.
2. Request shutdown on the control manager and memory manager. Poll both status
   registers until bit 1 is set. Keep all accelerator MMIO accesses stopped.
3. Copy the selected `cnn_bin` or `snn_bin` into a sufficiently large, 64-byte
   aligned DDR DMA buffer. Flush that buffer's data cache.
4. Use the Zynq `XDcfg` driver to transfer that binary through PCAP in partial
   reconfiguration mode. DMA lengths are in 32-bit words. Observe transfer errors
   and completion with a timeout; the DMA-done event alone is not configuration
   completion. The earlier LED demo's buffer limit is too small for these images.
5. The Pblock has `RESET_AFTER_RECONFIG=true`, so initialization is applied after
   partial configuration. Re-enable the memory path first, then the control path,
   checking that each manager has returned to pass-through.
6. Check accelerator readiness and initialize its model-specific controls before
   updating `active_model` and acknowledging READY to the PC.

If shutdown times out, leave both shutdown requests asserted and do not start
PCAP. On a configuration failure, keep the acknowledged isolation asserted.
On a release/readiness failure, re-request isolation and require recovery.
Never report READY or resume inference on these failures, and do not access
accelerator registers while isolated.

The first FPGA programming step must load the matching full parent bitstream.
The parent configuration starts with CNN. Subsequent switches use the partials.

## Inputs and results are model-specific

- SNN input: 384 signed 16-bit features, 768 bytes, with the existing ON/OFF
  feature extractor and scale 128. SNN output logits are four 32-bit values.
- CNN input storage: 4800 signed 16-bit Q8.8 values, 9600 bytes. The shared sender
  uses the original CNN sender's normalization, confirmed for this project.
  CNN logits are four signed 16-bit values, packed two per 32-bit AXI word and
  sign-extended into the shared int32 result format.
- The existing `send_images_uart_snn.py` prepares SNN features; it does not provide
  the CNN preprocessing branch or a model-selection command yet.

The shared sender implements acknowledged SELECT_MODEL and one inference at a
time, automatically preparing the selected model's payload. CNN class IDs are Circle, Square, Triangle, Figure_Eight; SNN class IDs remain
Circle, Figure_Eight, Square, Triangle. Switching and
inference times are reported separately. The original standalone SNN sender
remains for the old application; it does not speak the new shared protocol.
