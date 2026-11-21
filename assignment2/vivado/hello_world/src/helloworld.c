#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "nn_protocol.h"
#include "dfx_runtime.h"
#include "xparameters.h"
#include "xuartps.h"
#include "xuartps_hw.h"
#include "xiltimer.h"

#define UART_BAUD 4000000U
#define RX_TIMEOUT_US 2000000U
static XUartPs uart;
static uint8_t frame[NN_HEADER_BYTES + NN_MAX_PAYLOAD + 2U];

static XTime clock_now(void) { XTime t; XTime_GetTime(&t); return t; }
static int expired(XTime start, uint32_t us)
{
    return (uint64_t)(clock_now() - start) >= ((uint64_t)(COUNTS_PER_SECOND) * us) / 1000000U;
}

/* A whole frame has a bounded deadline; the idle command loop may wait forever. */
static int receive_byte(uint8_t *value, XTime start)
{
    while (!XUartPs_IsReceiveData(uart.Config.BaseAddress))
        if (expired(start, RX_TIMEOUT_US)) return NN_TIMEOUT;
    *value = (uint8_t)XUartPs_ReadReg(uart.Config.BaseAddress, XUARTPS_FIFO_OFFSET);
    return NN_OK;
}

static void wait_magic(void)
{
    unsigned seen = 0;
    for (;;) {
        if (!XUartPs_IsReceiveData(uart.Config.BaseAddress)) continue;
        uint8_t b = (uint8_t)XUartPs_ReadReg(uart.Config.BaseAddress, XUARTPS_FIFO_OFFSET);
        if (seen && b == 0x55U) return;
        seen = b == 0xaaU;
    }
}

static int receive_frame(void)
{
    memset(frame, 0, NN_HEADER_BYTES);
    XTime start = clock_now();
    for (unsigned i = 0; i < NN_HEADER_BYTES; ++i)
        if (receive_byte(frame + i, start)) return NN_TIMEOUT;
    unsigned bytes = nn_get16(frame + 6);
    if (bytes > NN_MAX_PAYLOAD) {
        /* Discard this oversized frame until a short UART idle gap. */
        XTime quiet = clock_now();
        while (!expired(quiet, 10000U) && !expired(start, RX_TIMEOUT_US)) {
            if (XUartPs_IsReceiveData(uart.Config.BaseAddress)) {
                (void)XUartPs_ReadReg(uart.Config.BaseAddress, XUARTPS_FIFO_OFFSET);
                quiet = clock_now();
            }
        }
        return NN_BAD_LENGTH;
    }
    for (unsigned i = 0; i < bytes + 2U; ++i)
        if (receive_byte(frame + NN_HEADER_BYTES + i, start)) return NN_TIMEOUT;
    int status = nn_validate(frame, NN_HEADER_BYTES + bytes + 2U);
    if (!status && frame[3] != 0) status = NN_BAD_COMMAND;
    return status;
}

static void send_reply(uint8_t command, uint8_t model, uint16_t sequence,
                       uint8_t status, const uint8_t *payload, uint16_t length)
{
    uint8_t reply[NN_HEADER_BYTES + 21U + 2U] = {NN_VERSION, command, model, status};
    nn_put16(reply + 4, sequence);
    nn_put16(reply + 6, length);
    if (length) memcpy(reply + NN_HEADER_BYTES, payload, length);
    nn_put16(reply + NN_HEADER_BYTES + length, nn_crc16(reply, NN_HEADER_BYTES + length));
    XUartPs_SendByte(uart.Config.BaseAddress, 0xa5);
    XUartPs_SendByte(uart.Config.BaseAddress, 0x5a);
    for (unsigned i = 0; i < NN_HEADER_BYTES + length + 2U; ++i)
        XUartPs_SendByte(uart.Config.BaseAddress, reply[i]);
}

/* Echo reply: reuse the received frame in place (header + payload), so up to
 * NN_MAX_PAYLOAD bytes go back without a second buffer. */
static void send_echo(void)
{
    uint16_t length = nn_get16(frame + 6);
    frame[0] = NN_VERSION;
    frame[3] = NN_OK;
    nn_put16(frame + NN_HEADER_BYTES + length, nn_crc16(frame, NN_HEADER_BYTES + length));
    XUartPs_SendByte(uart.Config.BaseAddress, 0xa5);
    XUartPs_SendByte(uart.Config.BaseAddress, 0x5a);
    for (unsigned i = 0; i < NN_HEADER_BYTES + length + 2U; ++i)
        XUartPs_SendByte(uart.Config.BaseAddress, frame[i]);
}

int main(void)
{
    init_platform();
    XUartPs_Config *cfg = XUartPs_LookupConfig(STDIN_BASEADDRESS);
    if (!cfg || XUartPs_CfgInitialize(&uart, cfg, cfg->BaseAddress) != XST_SUCCESS)
        return 1;
    XUartPs_SetOperMode(&uart, XUARTPS_OPER_MODE_NORMAL);
    XUartPsFormat format = {.BaudRate = UART_BAUD, .DataBits = XUARTPS_FORMAT_8_BITS,
        .Parity = XUARTPS_FORMAT_NO_PARITY, .StopBits = XUARTPS_FORMAT_1_STOP_BIT};
    if (XUartPs_SetDataFormat(&uart, &format) != XST_SUCCESS) return 1;
    (void)dfx_init(); /* STATUS exposes FAILED if platform initialization failed. */
    /* Binary protocol only: no debug prints mixed into responses. */
    for (;;) {
        wait_magic();
        int status = receive_frame();
        uint8_t command = frame[1], model = frame[2];
        uint16_t sequence = nn_get16(frame + 4), length = nn_get16(frame + 6);
        uint8_t payload[21] = {0};
        uint16_t reply_length = 0;
        if (!status && command == NN_ECHO && model == NN_NONE) {
            send_echo();
            continue;
        }
        if (!status) {
            switch (command) {
            case NN_INFO: {
                if (length) { status = NN_BAD_LENGTH; break; }
                if (model != NN_NONE) { status = NN_BAD_MODEL; break; }
                XTime before = clock_now(), after = before;
                /* Iteration bound also works if the counter is stopped. */
                for (unsigned i = 0; i < 10000U && after == before; ++i)
                    after = clock_now();
                nn_put32(payload, NN_FIRMWARE_REVISION);
                nn_put32(payload + 4, (uint32_t)(COUNTS_PER_SECOND));
                nn_put32(payload + 8, (uint32_t)(after - before));
                reply_length = 12;
                break;
            }
            case NN_STATUS:
                if (length) { status = NN_BAD_LENGTH; break; }
                if (model != NN_NONE) { status = NN_BAD_MODEL; break; }
                model = dfx_model();
                payload[0] = dfx_state();
                reply_length = 1;
                break;
            case NN_SELECT: {
                if (length) { status = NN_BAD_LENGTH; break; }
                uint32_t switch_us;
                status = dfx_select(model, &switch_us);
                if (!status) { nn_put32(payload, switch_us); reply_length = 4; }
                break;
            }
            case NN_INFER: {
                uint8_t prediction;
                uint32_t inference_us;
                int32_t logits[4];
                status = dfx_infer(model, frame + NN_HEADER_BYTES, length,
                                   &prediction, &inference_us, logits);
                if (!status) {
                    payload[0] = prediction;
                    nn_put32(payload + 1, inference_us);
                    for (unsigned i = 0; i < 4; ++i)
                        nn_put32(payload + 5 + 4U * i, (uint32_t)logits[i]);
                    reply_length = 21;
                }
                break;
            }
            default: status = NN_BAD_COMMAND; break;
            }
        }
        send_reply(command, model, sequence, (uint8_t)status, payload, reply_length);
    }
}
