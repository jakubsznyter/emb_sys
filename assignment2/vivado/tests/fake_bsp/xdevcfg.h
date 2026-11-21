#ifndef TEST_DEVCFG_H
#define TEST_DEVCFG_H
#include "xdevcfg_hw.h" /* Real generated BSP masks, copied to a temp include dir. */
typedef struct { UINTPTR BaseAddr; } XDcfg_Config;
typedef struct { XDcfg_Config Config; } XDcfg;
#define XDCFG_NON_SECURE_PCAP_WRITE 1
XDcfg_Config *XDcfg_LookupConfig(UINTPTR address);
int XDcfg_CfgInitialize(XDcfg *, XDcfg_Config *, u32);
void XDcfg_EnablePCAP(XDcfg *);
void XDcfg_SelectPcapInterface(XDcfg *);
void XDcfg_IntrClear(XDcfg *, u32);
u32 XDcfg_IntrGetStatus(XDcfg *);
u32 XDcfg_IsDmaBusy(XDcfg *);
u32 XDcfg_Transfer(XDcfg *, void *, u32, void *, u32, u32);
#endif
