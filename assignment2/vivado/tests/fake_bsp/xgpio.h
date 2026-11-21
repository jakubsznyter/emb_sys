#include "xil_types.h"
typedef struct { int unused; } XGpio;
int XGpio_Initialize(XGpio *, UINTPTR);
void XGpio_SetDataDirection(XGpio *, unsigned, u32);
void XGpio_DiscreteWrite(XGpio *, unsigned, u32);
