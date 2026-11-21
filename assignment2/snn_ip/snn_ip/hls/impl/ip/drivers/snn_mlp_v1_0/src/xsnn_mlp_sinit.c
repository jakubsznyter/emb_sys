// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2026.1 (64-bit)
// Tool Version Limit: 2026.06
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#ifdef SDT
#include "xparameters.h"
#endif
#include "xsnn_mlp.h"

extern XSnn_mlp_Config XSnn_mlp_ConfigTable[];

#ifdef SDT
XSnn_mlp_Config *XSnn_mlp_LookupConfig(UINTPTR BaseAddress) {
	XSnn_mlp_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XSnn_mlp_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XSnn_mlp_ConfigTable[Index].Ctrl_BaseAddress == BaseAddress) {
			ConfigPtr = &XSnn_mlp_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XSnn_mlp_Initialize(XSnn_mlp *InstancePtr, UINTPTR BaseAddress) {
	XSnn_mlp_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XSnn_mlp_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XSnn_mlp_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XSnn_mlp_Config *XSnn_mlp_LookupConfig(u16 DeviceId) {
	XSnn_mlp_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XSNN_MLP_NUM_INSTANCES; Index++) {
		if (XSnn_mlp_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XSnn_mlp_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XSnn_mlp_Initialize(XSnn_mlp *InstancePtr, u16 DeviceId) {
	XSnn_mlp_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XSnn_mlp_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XSnn_mlp_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

