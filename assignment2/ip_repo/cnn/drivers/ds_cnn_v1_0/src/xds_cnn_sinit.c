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
#include "xds_cnn.h"

extern XDs_cnn_Config XDs_cnn_ConfigTable[];

#ifdef SDT
XDs_cnn_Config *XDs_cnn_LookupConfig(UINTPTR BaseAddress) {
	XDs_cnn_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XDs_cnn_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XDs_cnn_ConfigTable[Index].Control_BaseAddress == BaseAddress) {
			ConfigPtr = &XDs_cnn_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XDs_cnn_Initialize(XDs_cnn *InstancePtr, UINTPTR BaseAddress) {
	XDs_cnn_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XDs_cnn_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XDs_cnn_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XDs_cnn_Config *XDs_cnn_LookupConfig(u16 DeviceId) {
	XDs_cnn_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XDS_CNN_NUM_INSTANCES; Index++) {
		if (XDs_cnn_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XDs_cnn_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XDs_cnn_Initialize(XDs_cnn *InstancePtr, u16 DeviceId) {
	XDs_cnn_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XDs_cnn_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XDs_cnn_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

