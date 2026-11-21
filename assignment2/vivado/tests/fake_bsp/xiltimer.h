#include <stdint.h>
typedef uint64_t XTime;
/* Match the real BSP's unparenthesized CPU_FREQ / 2 macro. */
#define COUNTS_PER_SECOND 2000000ULL / 2
void XTime_GetTime(XTime *);
