// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2026.1 (64-bit)
// Tool Version Limit: 2026.06
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XDS_CNN_H
#define XDS_CNN_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xds_cnn_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
typedef struct {
#ifdef SDT
    char *Name;
#else
    u16 DeviceId;
#endif
    u64 Control_BaseAddress;
} XDs_cnn_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XDs_cnn;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XDs_cnn_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XDs_cnn_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XDs_cnn_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XDs_cnn_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
#ifdef SDT
int XDs_cnn_Initialize(XDs_cnn *InstancePtr, UINTPTR BaseAddress);
XDs_cnn_Config* XDs_cnn_LookupConfig(UINTPTR BaseAddress);
#else
int XDs_cnn_Initialize(XDs_cnn *InstancePtr, u16 DeviceId);
XDs_cnn_Config* XDs_cnn_LookupConfig(u16 DeviceId);
#endif
int XDs_cnn_CfgInitialize(XDs_cnn *InstancePtr, XDs_cnn_Config *ConfigPtr);
#else
int XDs_cnn_Initialize(XDs_cnn *InstancePtr, const char* InstanceName);
int XDs_cnn_Release(XDs_cnn *InstancePtr);
#endif

void XDs_cnn_Start(XDs_cnn *InstancePtr);
u32 XDs_cnn_IsDone(XDs_cnn *InstancePtr);
u32 XDs_cnn_IsIdle(XDs_cnn *InstancePtr);
u32 XDs_cnn_IsReady(XDs_cnn *InstancePtr);
void XDs_cnn_EnableAutoRestart(XDs_cnn *InstancePtr);
void XDs_cnn_DisableAutoRestart(XDs_cnn *InstancePtr);

void XDs_cnn_Set_input_img(XDs_cnn *InstancePtr, u64 Data);
u64 XDs_cnn_Get_input_img(XDs_cnn *InstancePtr);
u32 XDs_cnn_Get_pred_out(XDs_cnn *InstancePtr);
u32 XDs_cnn_Get_pred_out_vld(XDs_cnn *InstancePtr);
u32 XDs_cnn_Get_logits_out_BaseAddress(XDs_cnn *InstancePtr);
u32 XDs_cnn_Get_logits_out_HighAddress(XDs_cnn *InstancePtr);
u32 XDs_cnn_Get_logits_out_TotalBytes(XDs_cnn *InstancePtr);
u32 XDs_cnn_Get_logits_out_BitWidth(XDs_cnn *InstancePtr);
u32 XDs_cnn_Get_logits_out_Depth(XDs_cnn *InstancePtr);
u32 XDs_cnn_Write_logits_out_Words(XDs_cnn *InstancePtr, int offset, word_type *data, int length);
u32 XDs_cnn_Read_logits_out_Words(XDs_cnn *InstancePtr, int offset, word_type *data, int length);
u32 XDs_cnn_Write_logits_out_Bytes(XDs_cnn *InstancePtr, int offset, char *data, int length);
u32 XDs_cnn_Read_logits_out_Bytes(XDs_cnn *InstancePtr, int offset, char *data, int length);

void XDs_cnn_InterruptGlobalEnable(XDs_cnn *InstancePtr);
void XDs_cnn_InterruptGlobalDisable(XDs_cnn *InstancePtr);
void XDs_cnn_InterruptEnable(XDs_cnn *InstancePtr, u32 Mask);
void XDs_cnn_InterruptDisable(XDs_cnn *InstancePtr, u32 Mask);
void XDs_cnn_InterruptClear(XDs_cnn *InstancePtr, u32 Mask);
u32 XDs_cnn_InterruptGetEnabled(XDs_cnn *InstancePtr);
u32 XDs_cnn_InterruptGetStatus(XDs_cnn *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
