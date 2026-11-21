// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2026.1 (64-bit)
// Tool Version Limit: 2026.06
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XSNN_MLP_H
#define XSNN_MLP_H

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
#include "xsnn_mlp_hw.h"

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
    u64 Ctrl_BaseAddress;
} XSnn_mlp_Config;
#endif

typedef struct {
    u64 Ctrl_BaseAddress;
    u32 IsReady;
} XSnn_mlp;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XSnn_mlp_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XSnn_mlp_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XSnn_mlp_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XSnn_mlp_ReadReg(BaseAddress, RegOffset) \
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
int XSnn_mlp_Initialize(XSnn_mlp *InstancePtr, UINTPTR BaseAddress);
XSnn_mlp_Config* XSnn_mlp_LookupConfig(UINTPTR BaseAddress);
#else
int XSnn_mlp_Initialize(XSnn_mlp *InstancePtr, u16 DeviceId);
XSnn_mlp_Config* XSnn_mlp_LookupConfig(u16 DeviceId);
#endif
int XSnn_mlp_CfgInitialize(XSnn_mlp *InstancePtr, XSnn_mlp_Config *ConfigPtr);
#else
int XSnn_mlp_Initialize(XSnn_mlp *InstancePtr, const char* InstanceName);
int XSnn_mlp_Release(XSnn_mlp *InstancePtr);
#endif

void XSnn_mlp_Start(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_IsDone(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_IsIdle(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_IsReady(XSnn_mlp *InstancePtr);
void XSnn_mlp_EnableAutoRestart(XSnn_mlp *InstancePtr);
void XSnn_mlp_DisableAutoRestart(XSnn_mlp *InstancePtr);

void XSnn_mlp_Set_feature_mem(XSnn_mlp *InstancePtr, u64 Data);
u64 XSnn_mlp_Get_feature_mem(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_Get_prediction(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_Get_prediction_vld(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_Get_logits_out_BaseAddress(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_Get_logits_out_HighAddress(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_Get_logits_out_TotalBytes(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_Get_logits_out_BitWidth(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_Get_logits_out_Depth(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_Write_logits_out_Words(XSnn_mlp *InstancePtr, int offset, word_type *data, int length);
u32 XSnn_mlp_Read_logits_out_Words(XSnn_mlp *InstancePtr, int offset, word_type *data, int length);
u32 XSnn_mlp_Write_logits_out_Bytes(XSnn_mlp *InstancePtr, int offset, char *data, int length);
u32 XSnn_mlp_Read_logits_out_Bytes(XSnn_mlp *InstancePtr, int offset, char *data, int length);

void XSnn_mlp_InterruptGlobalEnable(XSnn_mlp *InstancePtr);
void XSnn_mlp_InterruptGlobalDisable(XSnn_mlp *InstancePtr);
void XSnn_mlp_InterruptEnable(XSnn_mlp *InstancePtr, u32 Mask);
void XSnn_mlp_InterruptDisable(XSnn_mlp *InstancePtr, u32 Mask);
void XSnn_mlp_InterruptClear(XSnn_mlp *InstancePtr, u32 Mask);
u32 XSnn_mlp_InterruptGetEnabled(XSnn_mlp *InstancePtr);
u32 XSnn_mlp_InterruptGetStatus(XSnn_mlp *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
