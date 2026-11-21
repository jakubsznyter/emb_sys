// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2026.1 (64-bit)
// Tool Version Limit: 2026.06
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
// CTRL
// 0x00 : Control signals
//        bit 0  - ap_start (Read/Write/COH)
//        bit 1  - ap_done (Read/COR)
//        bit 2  - ap_idle (Read)
//        bit 3  - ap_ready (Read/COR)
//        bit 7  - auto_restart (Read/Write)
//        bit 9  - interrupt (Read)
//        others - reserved
// 0x04 : Global Interrupt Enable Register
//        bit 0  - Global Interrupt Enable (Read/Write)
//        others - reserved
// 0x08 : IP Interrupt Enable Register (Read/Write)
//        bit 0 - enable ap_done interrupt (Read/Write)
//        bit 1 - enable ap_ready interrupt (Read/Write)
//        others - reserved
// 0x0c : IP Interrupt Status Register (Read/TOW)
//        bit 0 - ap_done (Read/TOW)
//        bit 1 - ap_ready (Read/TOW)
//        others - reserved
// 0x10 : Data signal of feature_mem
//        bit 31~0 - feature_mem[31:0] (Read/Write)
// 0x14 : Data signal of feature_mem
//        bit 31~0 - feature_mem[63:32] (Read/Write)
// 0x18 : reserved
// 0x1c : Data signal of prediction
//        bit 7~0 - prediction[7:0] (Read)
//        others  - reserved
// 0x20 : Control signal of prediction
//        bit 0  - prediction_ap_vld (Read/COR)
//        others - reserved
// 0x30 ~
// 0x3f : Memory 'logits_out' (4 * 32b)
//        Word n : bit [31:0] - logits_out[n]
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XSNN_MLP_CTRL_ADDR_AP_CTRL          0x00
#define XSNN_MLP_CTRL_ADDR_GIE              0x04
#define XSNN_MLP_CTRL_ADDR_IER              0x08
#define XSNN_MLP_CTRL_ADDR_ISR              0x0c
#define XSNN_MLP_CTRL_ADDR_FEATURE_MEM_DATA 0x10
#define XSNN_MLP_CTRL_BITS_FEATURE_MEM_DATA 64
#define XSNN_MLP_CTRL_ADDR_PREDICTION_DATA  0x1c
#define XSNN_MLP_CTRL_BITS_PREDICTION_DATA  8
#define XSNN_MLP_CTRL_ADDR_PREDICTION_CTRL  0x20
#define XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE  0x30
#define XSNN_MLP_CTRL_ADDR_LOGITS_OUT_HIGH  0x3f
#define XSNN_MLP_CTRL_WIDTH_LOGITS_OUT      32
#define XSNN_MLP_CTRL_DEPTH_LOGITS_OUT      4

