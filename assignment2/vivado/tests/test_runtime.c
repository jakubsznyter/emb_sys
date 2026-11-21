/* Execute the real runtime against deterministic MMIO/PCAP hardware fakes.
 * This exercises sequencing and failure handling, not FPGA electrical behavior. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "dfx_runtime.h"
#include "dfx_hw.h"
#include "nn_protocol.h"
#include "xdevcfg.h"
#include "xgpio.h"
#include "xparameters.h"
#include "xiltimer.h"

extern unsigned char cnn_bin[], snn_bin[];
extern unsigned int cnn_bin_len, snn_bin_len;
static XDcfg_Config cfg = {XPAR_DEVCFG_BASEADDR};
static unsigned control_req, memory_req, transfers, bram_writes, gpio;
static int shutdown_stall, release_stall, dma_error, dma_only, no_startup_done;
static int dma_busy, infer_stall, invalid_pred, not_idle, started, flushed;
static u32 irq;
static XTime ticks;
static uint8_t last_model;

void XTime_GetTime(XTime *t) { ticks += 100000; *t = ticks; }
u32 Xil_In32(UINTPTR address)
{
    if (address == DFX_CONTROL_SHUTDOWN_BASE || address == DFX_MEMORY_SHUTDOWN_BASE) {
        unsigned request = address == DFX_CONTROL_SHUTDOWN_BASE ? control_req : memory_req;
        if (request) return shutdown_stall ? 1 : 15;
        return release_stall ? 14 : 0;
    }
    if (address == XPAR_DEVCFG_BASEADDR + XDCFG_STATUS_OFFSET) return XDCFG_STATUS_PCFG_INIT_MASK;
    if (address == XPAR_DEVCFG_BASEADDR + XDCFG_MCTRL_OFFSET) return 0;
    assert(!control_req); /* No accelerator MMIO through isolation. */
    if (address == DFX_ACCELERATOR_BASE) {
        if (not_idle || (started && infer_stall)) return 0;
        u32 result = 4 | (started ? 2 : 0);
        started = 0;
        return result;
    }
    if (address == DFX_ACCELERATOR_BASE + 0x1c) return invalid_pred ? 255 : 2;
    if (address >= DFX_ACCELERATOR_BASE + 0x30 && address < DFX_ACCELERATOR_BASE + 0x40) {
        if (last_model == NN_CNN) return address == DFX_ACCELERATOR_BASE + 0x30 ? 0x7fff8000U : 0x0001ffffU;
        return (u32)(-1000 + (int)(address - DFX_ACCELERATOR_BASE - 0x30));
    }
    assert(0); return 0;
}
void Xil_Out32(UINTPTR address, u32 value)
{
    if (address == DFX_CONTROL_SHUTDOWN_BASE) { control_req = value; return; }
    if (address == DFX_MEMORY_SHUTDOWN_BASE) { memory_req = value; return; }
    if (address == XPAR_DEVCFG_BASEADDR + XDCFG_MCTRL_OFFSET) return;
    if (address >= DFX_INPUT_BRAM_BASE && address < DFX_INPUT_BRAM_BASE + DFX_INPUT_BRAM_BYTES) {
        ++bram_writes; return;
    }
    assert(!control_req);
    assert(address >= DFX_ACCELERATOR_BASE && address <= DFX_ACCELERATOR_BASE + 0x14);
    if (address == DFX_ACCELERATOR_BASE) started = value & 1;
}
void Xil_DCacheFlushRange(INTPTR address, u32 length)
{
    assert(((UINTPTR)address & 63U) == 0);
    assert(length == cnn_bin_len || length == snn_bin_len);
    flushed = 1;
}
XDcfg_Config *XDcfg_LookupConfig(UINTPTR address) { assert(address == cfg.BaseAddr); return &cfg; }
int XDcfg_CfgInitialize(XDcfg *d, XDcfg_Config *c, u32 address)
{ assert(address == c->BaseAddr); d->Config = *c; return XST_SUCCESS; }
void XDcfg_EnablePCAP(XDcfg *d) { (void)d; }
void XDcfg_SelectPcapInterface(XDcfg *d) { (void)d; }
void XDcfg_IntrClear(XDcfg *d, u32 mask) { (void)d; assert(mask == XDCFG_IXR_ALL_MASK); irq = 0; }
u32 XDcfg_IntrGetStatus(XDcfg *d) { (void)d; return irq; }
u32 XDcfg_IsDmaBusy(XDcfg *d) { (void)d; return dma_busy ? XST_SUCCESS : XST_FAILURE; }
u32 XDcfg_Transfer(XDcfg *d, void *source, u32 words, void *dest, u32 dest_words, u32 type)
{
    (void)d;
    assert(control_req && memory_req && !shutdown_stall && flushed && irq == 0);
    assert((UINTPTR)dest == XDCFG_DMA_INVALID_ADDRESS && dest_words == 0 && type == XDCFG_NON_SECURE_PCAP_WRITE);
    const unsigned char *image = last_model == NN_CNN ? cnn_bin : snn_bin;
    assert(words * 4U == (last_model == NN_CNN ? cnn_bin_len : snn_bin_len));
    assert(memcmp(source, image, words * 4U) == 0); /* No extra byte swap. */
    ++transfers;
    irq = dma_error ? XDCFG_IXR_DMA_CMD_ERR_MASK : XDCFG_IXR_DMA_DONE_MASK;
    if (!dma_only) irq |= XDCFG_IXR_D_P_DONE_MASK;
    if (!no_startup_done && !dma_only) irq |= XDCFG_IXR_PCFG_DONE_MASK;
    return XST_SUCCESS;
}
int XGpio_Initialize(XGpio *g, UINTPTR a) { (void)g; assert(a == DFX_TIMING_GPIO_BASE); return XST_SUCCESS; }
void XGpio_SetDataDirection(XGpio *g, unsigned c, u32 v) { (void)g; assert(c == 1 && v == 0); }
void XGpio_DiscreteWrite(XGpio *g, unsigned c, u32 v) { (void)g; assert(c == 1); gpio = v; }

static void reset(void)
{
    control_req = memory_req = transfers = bram_writes = gpio = 0;
    shutdown_stall = release_stall = dma_error = dma_only = no_startup_done = 0;
    dma_busy = infer_stall = invalid_pred = not_idle = started = flushed = 0;
    ticks = 0; irq = XDCFG_IXR_DMA_DONE_MASK | XDCFG_IXR_PCFG_DONE_MASK;
    last_model = NN_CNN;
    assert(dfx_init() == NN_OK && dfx_state() == NN_UNSELECTED && dfx_model() == NN_NONE);
}
static void check_fault(int expected, int actual)
{
    assert(actual == expected);
    assert(dfx_state() == NN_FAILED && dfx_model() == NN_NONE && control_req && memory_req && !gpio);
    uint32_t us;
    unsigned before = transfers;
    assert(dfx_select(NN_CNN, &us) == NN_FAULT && transfers == before);
}
int main(void)
{
    uint32_t us; uint8_t pred; int32_t logits[4];
    static const uint8_t input[NN_CNN_BYTES] = {0};
    reset();
    assert(dfx_infer(NN_CNN, input, NN_CNN_BYTES, &pred, &us, logits) == NN_NOT_READY);
    assert(dfx_select(99, &us) == NN_BAD_MODEL && !transfers);
    assert(dfx_select(NN_CNN, &us) == NN_OK && us > 0 && transfers == 1);
    assert(dfx_select(NN_CNN, &us) == NN_OK && us == 0 && transfers == 1);
    assert(dfx_infer(NN_SNN, input, NN_SNN_BYTES, &pred, &us, logits) == NN_NOT_READY);
    assert(dfx_infer(NN_CNN, input, 2, &pred, &us, logits) == NN_BAD_LENGTH && !bram_writes);
    assert(dfx_infer(NN_CNN, input, NN_CNN_BYTES, &pred, &us, logits) == NN_OK);
    assert(us == 200000U); /* 200000 fake timer ticks at 1 MHz, not 50000 us. */
    assert(pred == 2 && logits[0] == -32768 && logits[1] == 32767 && logits[2] == -1 && logits[3] == 1);
    assert(bram_writes == NN_CNN_BYTES / 4 && !gpio);
    last_model = NN_SNN;
    assert(dfx_select(NN_SNN, &us) == NN_OK && transfers == 2);
    assert(dfx_infer(NN_SNN, input, NN_SNN_BYTES, &pred, &us, logits) == NN_OK && logits[0] == -1000 && logits[3] == -988);
    last_model = NN_CNN;
    assert(dfx_select(NN_CNN, &us) == NN_OK && transfers == 3);
    /* Application-only restart must not trust the previously active RM. */
    assert(dfx_init() == NN_OK);
    assert(dfx_select(NN_CNN, &us) == NN_OK && transfers == 4);
    reset(); shutdown_stall = 1; check_fault(NN_TIMEOUT, dfx_select(NN_CNN, &us)); assert(!transfers);
    reset(); dma_busy = 1; check_fault(NN_HW_ERROR, dfx_select(NN_CNN, &us)); assert(!transfers);
    reset(); dma_error = 1; check_fault(NN_HW_ERROR, dfx_select(NN_CNN, &us));
    reset(); dma_only = 1; check_fault(NN_TIMEOUT, dfx_select(NN_CNN, &us));
    reset(); no_startup_done = 1; check_fault(NN_TIMEOUT, dfx_select(NN_CNN, &us));
    reset(); release_stall = 1; check_fault(NN_TIMEOUT, dfx_select(NN_CNN, &us));
    reset(); not_idle = 1; check_fault(NN_TIMEOUT, dfx_select(NN_CNN, &us));
    reset(); assert(dfx_select(NN_CNN, &us) == NN_OK); infer_stall = 1;
    check_fault(NN_TIMEOUT, dfx_infer(NN_CNN, input, NN_CNN_BYTES, &pred, &us, logits));
    reset(); assert(dfx_select(NN_CNN, &us) == NN_OK); invalid_pred = 1;
    check_fault(NN_HW_ERROR, dfx_infer(NN_CNN, input, NN_CNN_BYTES, &pred, &us, logits));
    puts("PASS: runtime switching, restart, output packing and isolation/PCAP/inference failure paths");
    return 0;
}
