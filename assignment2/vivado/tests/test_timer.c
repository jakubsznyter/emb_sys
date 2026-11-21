/* Run the real BSP timer/sleep implementation and the real app init_platform()
 * against a fake global-timer register bank. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "xiltimer.h"
#include "sleep.h"
void init_platform(void);
static uint64_t counter;
static u32 control;
static unsigned lower_resets, upper_resets;
u32 Xil_In32(UINTPTR address)
{
    if (control & 1U) counter += 5;
    if (address == 0xf8f00200U) return (u32)counter;
    if (address == 0xf8f00204U) return (u32)(counter >> 32);
    if (address == 0xf8f00208U) return control;
    assert(0); return 0;
}
void Xil_Out32(UINTPTR address, u32 value)
{
    if (address == 0xf8f00200U) {
        counter = (counter & 0xffffffff00000000ULL) | value; ++lower_resets;
    } else if (address == 0xf8f00204U) {
        counter = (counter & 0xffffffffULL) | ((uint64_t)value << 32); ++upper_resets;
    } else if (address == 0xf8f00208U) control = value;
    else assert(0);
}
int main(void)
{
    XTime before, after;
    XTime_GetTime(&before); XTime_GetTime(&after);
    assert(before == 0 && after == 0 && !control);
    init_platform();
    assert(control == 1 && lower_resets == 1 && upper_resets == 1);
    XTime_GetTime(&before);
    usleep(100);
    XTime_GetTime(&after);
    assert(after > before && after - before >= 100);
    assert(lower_resets == 1 && upper_resets == 1);
    /* Debugger restart with the timer stopped and constructors skipped. The
     * BSP's private IsSleepTimerStarted flag remains true across this test. */
    control = 0x500; /* stopped, stale prescaler */
    memset(&TimerInst, 0, sizeof(TimerInst));
    init_platform();
    assert(control == 1 && lower_resets == 1 && upper_resets == 1);
    XTime_GetTime(&before); XTime_GetTime(&after);
    assert(after > before);
    printf("PASS: real BSP counter starts at application init; measured delay %llu us; later sleep preserves time\n",
           (unsigned long long)((after - before) * 1000000ULL / (COUNTS_PER_SECOND)));
    return 0;
}
