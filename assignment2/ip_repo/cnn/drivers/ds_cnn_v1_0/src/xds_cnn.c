// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2026.1 (64-bit)
// Tool Version Limit: 2026.06
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xds_cnn.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XDs_cnn_CfgInitialize(XDs_cnn *InstancePtr, XDs_cnn_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XDs_cnn_Start(XDs_cnn *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_AP_CTRL) & 0x80;
    XDs_cnn_WriteReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XDs_cnn_IsDone(XDs_cnn *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XDs_cnn_IsIdle(XDs_cnn *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XDs_cnn_IsReady(XDs_cnn *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XDs_cnn_EnableAutoRestart(XDs_cnn *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDs_cnn_WriteReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XDs_cnn_DisableAutoRestart(XDs_cnn *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDs_cnn_WriteReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_AP_CTRL, 0);
}

void XDs_cnn_Set_input_img(XDs_cnn *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDs_cnn_WriteReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_INPUT_IMG_DATA, (u32)(Data));
    XDs_cnn_WriteReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_INPUT_IMG_DATA + 4, (u32)(Data >> 32));
}

u64 XDs_cnn_Get_input_img(XDs_cnn *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_INPUT_IMG_DATA);
    Data += (u64)XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_INPUT_IMG_DATA + 4) << 32;
    return Data;
}

u32 XDs_cnn_Get_pred_out(XDs_cnn *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_PRED_OUT_DATA);
    return Data;
}

u32 XDs_cnn_Get_pred_out_vld(XDs_cnn *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_PRED_OUT_CTRL);
    return Data & 0x1;
}

u32 XDs_cnn_Get_logits_out_BaseAddress(XDs_cnn *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return (InstancePtr->Control_BaseAddress + XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE);
}

u32 XDs_cnn_Get_logits_out_HighAddress(XDs_cnn *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return (InstancePtr->Control_BaseAddress + XDS_CNN_CONTROL_ADDR_LOGITS_OUT_HIGH);
}

u32 XDs_cnn_Get_logits_out_TotalBytes(XDs_cnn *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return (XDS_CNN_CONTROL_ADDR_LOGITS_OUT_HIGH - XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE + 1);
}

u32 XDs_cnn_Get_logits_out_BitWidth(XDs_cnn *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDS_CNN_CONTROL_WIDTH_LOGITS_OUT;
}

u32 XDs_cnn_Get_logits_out_Depth(XDs_cnn *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDS_CNN_CONTROL_DEPTH_LOGITS_OUT;
}

u32 XDs_cnn_Write_logits_out_Words(XDs_cnn *InstancePtr, int offset, word_type *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length)*4 > (XDS_CNN_CONTROL_ADDR_LOGITS_OUT_HIGH - XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(int *)(InstancePtr->Control_BaseAddress + XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE + (offset + i)*4) = *(data + i);
    }
    return length;
}

u32 XDs_cnn_Read_logits_out_Words(XDs_cnn *InstancePtr, int offset, word_type *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length)*4 > (XDS_CNN_CONTROL_ADDR_LOGITS_OUT_HIGH - XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(data + i) = *(int *)(InstancePtr->Control_BaseAddress + XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE + (offset + i)*4);
    }
    return length;
}

u32 XDs_cnn_Write_logits_out_Bytes(XDs_cnn *InstancePtr, int offset, char *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length) > (XDS_CNN_CONTROL_ADDR_LOGITS_OUT_HIGH - XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(char *)(InstancePtr->Control_BaseAddress + XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE + offset + i) = *(data + i);
    }
    return length;
}

u32 XDs_cnn_Read_logits_out_Bytes(XDs_cnn *InstancePtr, int offset, char *data, int length) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr -> IsReady == XIL_COMPONENT_IS_READY);

    int i;

    if ((offset + length) > (XDS_CNN_CONTROL_ADDR_LOGITS_OUT_HIGH - XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE + 1))
        return 0;

    for (i = 0; i < length; i++) {
        *(data + i) = *(char *)(InstancePtr->Control_BaseAddress + XDS_CNN_CONTROL_ADDR_LOGITS_OUT_BASE + offset + i);
    }
    return length;
}

void XDs_cnn_InterruptGlobalEnable(XDs_cnn *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDs_cnn_WriteReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_GIE, 1);
}

void XDs_cnn_InterruptGlobalDisable(XDs_cnn *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDs_cnn_WriteReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_GIE, 0);
}

void XDs_cnn_InterruptEnable(XDs_cnn *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_IER);
    XDs_cnn_WriteReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_IER, Register | Mask);
}

void XDs_cnn_InterruptDisable(XDs_cnn *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_IER);
    XDs_cnn_WriteReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_IER, Register & (~Mask));
}

void XDs_cnn_InterruptClear(XDs_cnn *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XDs_cnn_WriteReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_ISR, Mask);
}

u32 XDs_cnn_InterruptGetEnabled(XDs_cnn *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_IER);
}

u32 XDs_cnn_InterruptGetStatus(XDs_cnn *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XDs_cnn_ReadReg(InstancePtr->Control_BaseAddress, XDS_CNN_CONTROL_ADDR_ISR);
}

