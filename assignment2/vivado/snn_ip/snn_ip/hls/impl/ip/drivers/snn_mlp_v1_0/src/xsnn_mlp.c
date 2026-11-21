// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2026.1 (64-bit)
// Tool Version Limit: 2026.06
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xsnn_mlp.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XSnn_mlp_CfgInitialize(XSnn_mlp *InstancePtr, XSnn_mlp_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Ctrl_BaseAddress = ConfigPtr->Ctrl_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XSnn_mlp_Start(XSnn_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_AP_CTRL) & 0x80;
    XSnn_mlp_WriteReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XSnn_mlp_IsDone(XSnn_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XSnn_mlp_IsIdle(XSnn_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XSnn_mlp_IsReady(XSnn_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XSnn_mlp_EnableAutoRestart(XSnn_mlp *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSnn_mlp_WriteReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_AP_CTRL, 0x80);
}

void XSnn_mlp_DisableAutoRestart(XSnn_mlp *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSnn_mlp_WriteReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_AP_CTRL, 0);
}

void XSnn_mlp_Set_feature_mem(XSnn_mlp *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSnn_mlp_WriteReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_FEATURE_MEM_DATA, (u32)(Data));
    XSnn_mlp_WriteReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_FEATURE_MEM_DATA + 4, (u32)(Data >> 32));
}

u64 XSnn_mlp_Get_feature_mem(XSnn_mlp *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_FEATURE_MEM_DATA);
    Data += (u64)XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_FEATURE_MEM_DATA + 4) << 32;
    return Data;
}

u32 XSnn_mlp_Get_prediction(XSnn_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_PREDICTION_DATA);
    return Data;
}

u32 XSnn_mlp_Get_prediction_vld(XSnn_mlp *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_PREDICTION_CTRL);
    return Data & 0x1;
}

u32 XSnn_mlp_Get_logits_out_BaseAddress(XSnn_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return (InstancePtr->Ctrl_BaseAddress + XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE);
}

u32 XSnn_mlp_Get_logits_out_HighAddress(XSnn_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return (InstancePtr->Ctrl_BaseAddress + XSNN_MLP_CTRL_ADDR_LOGITS_OUT_HIGH);
}

u32 XSnn_mlp_Get_logits_out_TotalBytes(XSnn_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return (XSNN_MLP_CTRL_ADDR_LOGITS_OUT_HIGH - XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE + 1);
}

u32 XSnn_mlp_Get_logits_out_BitWidth(XSnn_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XSNN_MLP_CTRL_WIDTH_LOGITS_OUT;
}

u32 XSnn_mlp_Get_logits_out_Depth(XSnn_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XSNN_MLP_CTRL_DEPTH_LOGITS_OUT;
}

u32 XSnn_mlp_Write_logits_out_Words(XSnn_mlp *InstancePtr, int offset, word_type *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length)*4 > (XSNN_MLP_CTRL_ADDR_LOGITS_OUT_HIGH - XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(int *)(InstancePtr->Ctrl_BaseAddress + XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE + (offset + i)*4) = *(data + i);
    }
    return length;
}

u32 XSnn_mlp_Read_logits_out_Words(XSnn_mlp *InstancePtr, int offset, word_type *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length)*4 > (XSNN_MLP_CTRL_ADDR_LOGITS_OUT_HIGH - XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(data + i) = *(int *)(InstancePtr->Ctrl_BaseAddress + XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE + (offset + i)*4);
    }
    return length;
}

u32 XSnn_mlp_Write_logits_out_Bytes(XSnn_mlp *InstancePtr, int offset, char *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length) > (XSNN_MLP_CTRL_ADDR_LOGITS_OUT_HIGH - XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(char *)(InstancePtr->Ctrl_BaseAddress + XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE + offset + i) = *(data + i);
    }
    return length;
}

u32 XSnn_mlp_Read_logits_out_Bytes(XSnn_mlp *InstancePtr, int offset, char *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length) > (XSNN_MLP_CTRL_ADDR_LOGITS_OUT_HIGH - XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(data + i) = *(char *)(InstancePtr->Ctrl_BaseAddress + XSNN_MLP_CTRL_ADDR_LOGITS_OUT_BASE + offset + i);
    }
    return length;
}

void XSnn_mlp_InterruptGlobalEnable(XSnn_mlp *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSnn_mlp_WriteReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_GIE, 1);
}

void XSnn_mlp_InterruptGlobalDisable(XSnn_mlp *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSnn_mlp_WriteReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_GIE, 0);
}

void XSnn_mlp_InterruptEnable(XSnn_mlp *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_IER);
    XSnn_mlp_WriteReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_IER, Register | Mask);
}

void XSnn_mlp_InterruptDisable(XSnn_mlp *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_IER);
    XSnn_mlp_WriteReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_IER, Register & (~Mask));
}

void XSnn_mlp_InterruptClear(XSnn_mlp *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XSnn_mlp_WriteReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_ISR, Mask);
}

u32 XSnn_mlp_InterruptGetEnabled(XSnn_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_IER);
}

u32 XSnn_mlp_InterruptGetStatus(XSnn_mlp *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XSnn_mlp_ReadReg(InstancePtr->Ctrl_BaseAddress, XSNN_MLP_CTRL_ADDR_ISR);
}

