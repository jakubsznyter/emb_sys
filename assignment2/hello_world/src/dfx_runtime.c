#include <string.h>
#include "dfx_runtime.h"
#include "dfx_hw.h"
#include "nn_protocol.h"
#include "xdevcfg.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xiltimer.h"
#include "xds_cnn_hw.h"

/* Include each image exactly once. These are SMAPx32 BIN bytes, already in
 * PCAP word order: copy them unchanged, with no additional byte/bit swapping. */
#include "cnn.h"
#include "snn.h"

#define WAIT_US 1000000U
#define PCAP_WAIT_US 5000000U
/* Both packaged HLS register maps use these offsets. */
#define AP_CTRL 0x00U
#define INPUT_LO 0x10U
#define INPUT_HI 0x14U
#define PREDICTION 0x1cU
#define LOGITS 0x30U
#define AP_IDLE (1U << 2)
#define AP_DONE (1U << 1)

_Static_assert(DFX_ACCELERATOR_BASE == XPAR_CNN_0_DS_CNN_0_BASEADDR, "Wrong DFX BSP");
_Static_assert(DFX_CONTROL_SHUTDOWN_BASE == XPAR_NN_CONTROL_SHUTDOWN_BASEADDR, "Wrong control shutdown address");
_Static_assert(DFX_MEMORY_SHUTDOWN_BASE == XPAR_NN_MEMORY_SHUTDOWN_BASEADDR, "Wrong memory shutdown address");
_Static_assert(XPAR_AXI_BRAM_CTRL_0_HIGHADDR - XPAR_AXI_BRAM_CTRL_0_BASEADDR + 1U == DFX_INPUT_BRAM_BYTES,
               "Wrong input BRAM size");
_Static_assert(INPUT_LO == XDS_CNN_CONTROL_ADDR_INPUT_IMG_DATA &&
               PREDICTION == XDS_CNN_CONTROL_ADDR_PRED_OUT_DATA &&
               LOGITS == XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE, "Wrong CNN register layout");

static XDcfg config;
static XGpio timing;
static uint8_t active_model = NN_NONE;
static uint8_t state = NN_FAILED;
static uint8_t dma_buffer[(sizeof(cnn_bin) > sizeof(snn_bin)) ?
                          sizeof(cnn_bin) : sizeof(snn_bin)] __attribute__((aligned(64)));

static XTime now(void) { XTime t; XTime_GetTime(&t); return t; }
static uint32_t elapsed(XTime start)
{
    /* BSP COUNTS_PER_SECOND expands to an unparenthesized CPU_FREQ / 2. */
    return (uint32_t)(((uint64_t)(now() - start) * 1000000ULL) /
                      (uint64_t)(COUNTS_PER_SECOND));
}

static int wait_bits(UINTPTR address, uint32_t mask, uint32_t value)
{
    XTime start = now();
    do {
        if ((Xil_In32(address) & mask) == value) return NN_OK;
    } while (elapsed(start) < WAIT_US);
    return NN_TIMEOUT;
}

static void request_isolation(void)
{
    Xil_Out32(DFX_CONTROL_SHUTDOWN_BASE, DFX_REQUEST_SHUTDOWN);
    Xil_Out32(DFX_MEMORY_SHUTDOWN_BASE, DFX_REQUEST_SHUTDOWN);
}

static int fail(int error)
{
    request_isolation();
    XGpio_DiscreteWrite(&timing, 1, 0);
    active_model = NN_NONE;
    state = NN_FAILED;
    return error;
}

static int load_partial(uint8_t model)
{
    const uint8_t *image = model == NN_CNN ? cnn_bin : snn_bin;
    uint32_t bytes = model == NN_CNN ? cnn_bin_len : snn_bin_len;
    if (!bytes || bytes > sizeof(dma_buffer) || (bytes & 3U)) return NN_HW_ERROR;
    memcpy(dma_buffer, image, bytes);
    Xil_DCacheFlushRange((INTPTR)dma_buffer, bytes);

    /* Do not toggle PROG_B, PS/PL level shifters or the static reset here. */
    XDcfg_EnablePCAP(&config);
    XDcfg_SelectPcapInterface(&config);
    XDcfg_WriteReg(config.Config.BaseAddr, XDCFG_MCTRL_OFFSET,
        XDcfg_ReadReg(config.Config.BaseAddr, XDCFG_MCTRL_OFFSET) & ~XDCFG_MCTRL_PCAP_LPBK_MASK);
    XDcfg_IntrClear(&config, XDCFG_IXR_ALL_MASK);
    /* This driver returns XST_SUCCESS when BUSY, not when idle. */
    if (XDcfg_IsDmaBusy(&config) == XST_SUCCESS) return NN_HW_ERROR;
    if (XDcfg_Transfer(&config, dma_buffer, bytes / 4U,
                      (void *)XDCFG_DMA_INVALID_ADDRESS, 0,
                      XDCFG_NON_SECURE_PCAP_WRITE) != XST_SUCCESS) return NN_HW_ERROR;
    XTime start = now();
    const uint32_t done = XDCFG_IXR_DMA_DONE_MASK | XDCFG_IXR_D_P_DONE_MASK |
                          XDCFG_IXR_PCFG_DONE_MASK;
    /* RESET_AFTER_RECONFIG is enabled: wait for both transfer completion and
     * configuration/startup DONE, never just the DMA event. */
    do {
        uint32_t irq = XDcfg_IntrGetStatus(&config);
        if (irq & XDCFG_IXR_ERROR_FLAGS_MASK)
            return NN_HW_ERROR;
        if ((irq & done) == done) {
            return (XDcfg_ReadReg(config.Config.BaseAddr, XDCFG_STATUS_OFFSET) &
                    XDCFG_STATUS_PCFG_INIT_MASK) ? NN_OK : NN_HW_ERROR;
        }
    } while (elapsed(start) < PCAP_WAIT_US);
    return NN_TIMEOUT;
}

int dfx_init(void)
{
    XDcfg_Config *cfg = XDcfg_LookupConfig(XPAR_DEVCFG_BASEADDR);
    if (!cfg || XDcfg_CfgInitialize(&config, cfg, cfg->BaseAddr) != XST_SUCCESS)
        return NN_HW_ERROR;
    if (XGpio_Initialize(&timing, DFX_TIMING_GPIO_BASE) != XST_SUCCESS)
        return NN_HW_ERROR;
    XGpio_SetDataDirection(&timing, 1, 0);
    XGpio_DiscreteWrite(&timing, 1, 0);
    /* Do not infer the active RM from software state after an application reset.
     * The first SELECT always isolates and reloads the requested partial. */
    active_model = NN_NONE;
    state = NN_UNSELECTED;
    return NN_OK;
}

uint8_t dfx_model(void) { return active_model; }
uint8_t dfx_state(void) { return state; }

int dfx_select(uint8_t model, uint32_t *switch_us)
{
    *switch_us = 0;
    if (!nn_input_bytes(model)) return NN_BAD_MODEL;
    if (state == NN_FAILED) return NN_FAULT;
    if (state == NN_READY && active_model == model) return NN_OK;
    XTime start = now();
    int status;
    if (state == NN_READY) {
        /* Stop auto-restart before checking idle. No inference runs in parallel. */
        Xil_Out32(DFX_ACCELERATOR_BASE + AP_CTRL, 0);
        status = wait_bits(DFX_ACCELERATOR_BASE + AP_CTRL, AP_IDLE, AP_IDLE);
        if (status) return fail(status);
    }
    request_isolation();
    status = wait_bits(DFX_CONTROL_SHUTDOWN_BASE, DFX_STATUS_IN_SHUTDOWN,
                       DFX_STATUS_IN_SHUTDOWN);
    if (!status) status = wait_bits(DFX_MEMORY_SHUTDOWN_BASE,
                                   DFX_STATUS_IN_SHUTDOWN, DFX_STATUS_IN_SHUTDOWN);
    if (status) return fail(status);
    status = load_partial(model);
    if (status) return fail(status);
    /* Memory first, then control. Wait for all channel status bits to clear. */
    const uint32_t shutdown_bits = DFX_STATUS_REQUESTED | DFX_STATUS_IN_SHUTDOWN |
                                  DFX_STATUS_WRITE_SHUTDOWN | DFX_STATUS_READ_SHUTDOWN;
    Xil_Out32(DFX_MEMORY_SHUTDOWN_BASE, DFX_PASS_THROUGH);
    status = wait_bits(DFX_MEMORY_SHUTDOWN_BASE, shutdown_bits, 0);
    if (status) return fail(status);
    Xil_Out32(DFX_CONTROL_SHUTDOWN_BASE, DFX_PASS_THROUGH);
    status = wait_bits(DFX_CONTROL_SHUTDOWN_BASE, shutdown_bits, 0);
    if (status) return fail(status);
    status = wait_bits(DFX_ACCELERATOR_BASE + AP_CTRL, AP_IDLE, AP_IDLE);
    if (status) return fail(status);
    Xil_Out32(DFX_ACCELERATOR_BASE + AP_CTRL, 0);
    Xil_Out32(DFX_ACCELERATOR_BASE + INPUT_LO, DFX_INPUT_BRAM_BASE);
    Xil_Out32(DFX_ACCELERATOR_BASE + INPUT_HI, 0);
    active_model = model;
    state = NN_READY;
    *switch_us = elapsed(start);
    return NN_OK;
}

int dfx_infer(uint8_t model, const uint8_t *input, uint16_t length,
              uint8_t *prediction, uint32_t *inference_us, int32_t logits[4])
{
    if (!nn_input_bytes(model)) return NN_BAD_MODEL;
    if (length != nn_input_bytes(model)) return NN_BAD_LENGTH;
    if (state == NN_FAILED) return NN_FAULT;
    if (state != NN_READY || model != active_model) return NN_NOT_READY;
    if (wait_bits(DFX_ACCELERATOR_BASE + AP_CTRL, AP_IDLE, AP_IDLE)) return fail(NN_TIMEOUT);
    /* BRAM is device memory in the Cortex-A9 BSP mapping, not cached DDR. */
    for (uint32_t i = 0; i < length; i += 4U)
        Xil_Out32(DFX_INPUT_BRAM_BASE + i, nn_get32(input + i));
    Xil_Out32(DFX_ACCELERATOR_BASE + INPUT_LO, DFX_INPUT_BRAM_BASE);
    Xil_Out32(DFX_ACCELERATOR_BASE + INPUT_HI, 0);
    (void)Xil_In32(DFX_ACCELERATOR_BASE + AP_CTRL); /* Clear stale ap_done. */
    XTime start = now();
    XGpio_DiscreteWrite(&timing, 1, 1);
    Xil_Out32(DFX_ACCELERATOR_BASE + AP_CTRL, 1);
    int status = wait_bits(DFX_ACCELERATOR_BASE + AP_CTRL, AP_DONE, AP_DONE);
    XGpio_DiscreteWrite(&timing, 1, 0);
    *inference_us = elapsed(start);
    if (status) return fail(status);
    uint32_t pred = Xil_In32(DFX_ACCELERATOR_BASE + PREDICTION);
    if (pred > 3U) return fail(NN_HW_ERROR);
    *prediction = (uint8_t)pred;
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t word = Xil_In32(DFX_ACCELERATOR_BASE + LOGITS +
                                (model == NN_CNN ? (i / 2U) * 4U : i * 4U));
        logits[i] = model == NN_CNN ? nn_logit16(word, i & 1U) : (int32_t)word;
    }
    return NN_OK;
}
