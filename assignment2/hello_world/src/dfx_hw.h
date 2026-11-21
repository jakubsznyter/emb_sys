#ifndef ASSIGNMENT2_DFX_HW_H
#define ASSIGNMENT2_DFX_HW_H

/* Matching isolated DFX hardware. Both models share the accelerator base;
 * the static shutdown managers remain accessible while the RP is isolated. */
#define DFX_INPUT_BRAM_BASE       0x40000000U
#define DFX_INPUT_BRAM_BYTES      16384U
#define DFX_TIMING_GPIO_BASE      0x40010000U
#define DFX_CONTROL_SHUTDOWN_BASE 0x40020000U
#define DFX_MEMORY_SHUTDOWN_BASE  0x40030000U
#define DFX_ACCELERATOR_BASE      0x44A00000U

/* PG377: CONTROL (write) and STATUS (read) share offset zero. */
#define DFX_SHUTDOWN_REG_OFFSET   0x00U
#define DFX_REQUEST_SHUTDOWN      (1U << 0)
#define DFX_PASS_THROUGH          0U
#define DFX_STATUS_REQUESTED      (1U << 0)
#define DFX_STATUS_IN_SHUTDOWN    (1U << 1)
#define DFX_STATUS_WRITE_SHUTDOWN (1U << 2)
#define DFX_STATUS_READ_SHUTDOWN  (1U << 3)

#endif
