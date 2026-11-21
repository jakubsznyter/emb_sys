#ifndef TEST_DEFAULT_TIMER_H
#define TEST_DEFAULT_TIMER_H
#include <stdint.h>
#include <stddef.h>
#include "xil_io.h"
#define XTIMER_IS_DEFAULT_TIMER 1
#define XTIMER_NO_TICK_TIMER 1
#define FALSE 0
#define TRUE 1
#define COUNTS_PER_SECOND 2000000ULL / 2

typedef uint64_t XTime;
typedef enum { XTIMER_DELAY_SEC = 1, XTIMER_DELAY_MSEC = 1000,
               XTIMER_DELAY_USEC = 1000000 } XTimer_DelayType;
typedef void (*XTimer_TickHandler)(void *, u32);
typedef struct XTimerTag {
    void (*XTimer_ModifyInterval)(struct XTimerTag *, u32, XTimer_DelayType);
    void (*XTimer_TickIntrHandler)(struct XTimerTag *, u8);
    void (*XTimer_TickInterval)(struct XTimerTag *, u32);
    void (*XSleepTimer_Stop)(struct XTimerTag *);
    void (*XTickTimer_Stop)(struct XTimerTag *);
    void (*XTickTimer_ClearInterrupt)(struct XTimerTag *);
    XTimer_TickHandler Handler;
    void *CallBackRef;
} XTimer;
extern XTimer TimerInst;
u32 XilSleepTimer_Init(XTimer *);
u32 XilTickTimer_Init(XTimer *);
void XTime_GetTime(XTime *);
#endif
