#include <stdio.h>
#include <stdint.h>

#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xparameters.h"
#include "xiltimer.h"
#include "xuartps.h"

// ============================================================
// UART (4 Mbaud) - musi zgadzac sie z BAUD_RATE w skrypcie Pythona
// ============================================================
#ifndef UART_DEVICE_ID
#define UART_DEVICE_ID       XPAR_XUARTPS_0_BASEADDR
#endif
#define UART_BAUD_RATE       4000000U

// ============================================================
// Wejscie SNN: 384 cech INT16 (little-endian)
// ============================================================
#define N_INPUTS             384
#define RX_BYTES             (N_INPUTS * 2)   // 768 B

#define FRAME_START_0        0xAA
#define FRAME_START_1        0x55

// ============================================================
// Adresy sprzetowe
// ============================================================
#define IP_BASE_ADDR         XPAR_SNN_MLP_0_BASEADDR   // 0x40020000
#define FEATURE_BRAM_BASE    0x40000000U               // adres BRAM widziany przez PS i przez IP

// Offsety rejestrow HLS - SPRAWDZ w xsnn_mlp_hw.h w BSP!
#define REG_AP_CTRL          0x00
#define AP_START_BIT         0
#define AP_DONE_BIT          1
#define AP_IDLE_BIT          2

#define REG_FEATURE_MEM_L    0x10
#define REG_FEATURE_MEM_H    0x14
#define REG_PRED_OUT         0x1C
#define REG_LOGITS_BASE      0x30

// 1 = po zapisie czytaj BRAM z powrotem i porownuj (diagnostyka); 0 = wylacz
#define DEBUG_BRAM_READBACK  1

// Limit czekania na ap_done (liczba odczytow rejestru) - chroni przed zawieszeniem
#define DONE_TIMEOUT_POLLS   20000000U

static inline void ip_write(uint32_t off, uint32_t val)
{
    Xil_Out32(IP_BASE_ADDR + off, val);
}

static inline uint32_t ip_read(uint32_t off)
{
    return Xil_In32(IP_BASE_ADDR + off);
}

// ============================================================
// UART
// ============================================================
static XUartPs UartInst;

static int uart_init(uint32_t baud)
{
    XUartPs_Config *Cfg = XUartPs_LookupConfig(UART_DEVICE_ID);
    if (!Cfg) return XST_FAILURE;

    int st = XUartPs_CfgInitialize(&UartInst, Cfg, Cfg->BaseAddress);
    if (st != XST_SUCCESS) return st;

    XUartPs_SetOperMode(&UartInst, XUARTPS_OPER_MODE_NORMAL);
    return XUartPs_SetBaudRate(&UartInst, baud);
}

static inline uint8_t read_byte_blocking(void)
{
    while (!XUartPs_IsReceiveData(UartInst.Config.BaseAddress));
    return (uint8_t)XUartPs_ReadReg(UartInst.Config.BaseAddress, XUARTPS_FIFO_OFFSET);
}

static inline void write_byte_blocking(uint8_t byte)
{
    while (XUartPs_IsTransmitFull(UartInst.Config.BaseAddress));
    XUartPs_WriteReg(UartInst.Config.BaseAddress, XUARTPS_FIFO_OFFSET, byte);
}

static void wait_for_frame_header(void)
{
    while (1) {
        if (read_byte_blocking() == FRAME_START_0) {
            if (read_byte_blocking() == FRAME_START_1) return;
        }
    }
}

// Ramka odpowiedzi 23 B: [0xA5][pred][time_us 4B LE][4x int32 LE][0x5A]
static void uart_send_reply_23_bytes(uint8_t pred, uint32_t time_us,
                                     int32_t l0, int32_t l1, int32_t l2, int32_t l3)
{
    write_byte_blocking(0xA5);
    write_byte_blocking(pred);

    for (int s = 0; s < 32; s += 8)
        write_byte_blocking((uint8_t)((time_us >> s) & 0xFF));

    int32_t logits[4] = {l0, l1, l2, l3};
    for (int i = 0; i < 4; i++) {
        uint32_t v = (uint32_t)logits[i];
        for (int s = 0; s < 32; s += 8)
            write_byte_blocking((uint8_t)((v >> s) & 0xFF));
    }

    write_byte_blocking(0x5A);
}

static uint8_t rx_buf[RX_BYTES];

// ============================================================
// MAIN
// ============================================================
int main(void)
{
    uart_init(UART_BAUD_RATE);

    xil_printf("\r\n--- SNN Accelerator Controller Ready ---\r\n");
    xil_printf("IP AP_CTRL = 0x%08lX (bit2 = idle)\r\n", (unsigned long)ip_read(REG_AP_CTRL));

    while (1)
    {
        // 1. Naglowek 0xAA 0x55
        wait_for_frame_header();

        // 2. 768 bajtow cech
        for (int i = 0; i < RX_BYTES; i++) {
            rx_buf[i] = read_byte_blocking();
        }

        // 3. Zapis do BRAM: 2 x int16 na 32-bitowe slowo (LE: niższa cecha w dolnej polowie)
        for (int i = 0; i < N_INPUTS / 2; i++) {
            uint16_t low16  = ((uint16_t)rx_buf[(2 * i) * 2 + 1] << 8) | rx_buf[(2 * i) * 2 + 0];
            uint16_t high16 = ((uint16_t)rx_buf[(2 * i + 1) * 2 + 1] << 8) | rx_buf[(2 * i + 1) * 2 + 0];
            uint32_t packed = ((uint32_t)high16 << 16) | low16;
            Xil_Out32(FEATURE_BRAM_BASE + (i * 4), packed);
        }

#if DEBUG_BRAM_READBACK
        {
            int bad = 0;
            for (int i = 0; i < N_INPUTS / 2; i++) {
                uint16_t low16  = ((uint16_t)rx_buf[(2 * i) * 2 + 1] << 8) | rx_buf[(2 * i) * 2 + 0];
                uint16_t high16 = ((uint16_t)rx_buf[(2 * i + 1) * 2 + 1] << 8) | rx_buf[(2 * i + 1) * 2 + 0];
                uint32_t exp = ((uint32_t)high16 << 16) | low16;
                if (Xil_In32(FEATURE_BRAM_BASE + (i * 4)) != exp) bad++;
            }
            if (bad) xil_printf("BRAM readback mismatch: %d slow\r\n", bad);
        }
#endif

        // BRAM w 0x4000_0000 jest mapowany jako urzadzenie (bez cache) - flush/invalidate zbedny.

        // 4. Adres cech do IP (64-bitowy wskaznik)
        ip_write(REG_FEATURE_MEM_L, FEATURE_BRAM_BASE);
        ip_write(REG_FEATURE_MEM_H, 0U);

        // 5. Start i pomiar czasu
        XTime t0, t1;
        XTime_GetTime(&t0);
        ip_write(REG_AP_CTRL, (1U << AP_START_BIT));

        uint32_t polls = 0;
        while (((ip_read(REG_AP_CTRL) >> AP_DONE_BIT) & 0x1U) == 0U) {
            if (++polls > DONE_TIMEOUT_POLLS) {
                xil_printf("TIMEOUT: IP nie zglosilo ap_done (sprawdz adresy w Address Editor)\r\n");
                break;
            }
        }
        XTime_GetTime(&t1);

        uint64_t dt_ticks = (uint64_t)(t1 - t0);
        uint32_t time_us = (uint32_t)(dt_ticks / (COUNTS_PER_SECOND / 1000000UL));

        // 6. Wyniki
        uint32_t pred = ip_read(REG_PRED_OUT) & 0xFFU;
        int32_t l0 = (int32_t)ip_read(REG_LOGITS_BASE + 0x0);
        int32_t l1 = (int32_t)ip_read(REG_LOGITS_BASE + 0x4);
        int32_t l2 = (int32_t)ip_read(REG_LOGITS_BASE + 0x8);
        int32_t l3 = (int32_t)ip_read(REG_LOGITS_BASE + 0xC);

        // 7. Odpowiedz do Pythona
        uart_send_reply_23_bytes((uint8_t)pred, time_us, l0, l1, l2, l3);
    }

    return 0;
}