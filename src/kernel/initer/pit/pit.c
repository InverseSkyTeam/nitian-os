/**
 * 参考: https://wiki.osdev.org/Programmable_Interval_Timer
 */

#include "pit.h"
#include "../../include/asmFunc.h"

#define PIT_BASE_FREQ 1193182

void initPIT(uint32_t hz) {
    uint32_t divisor = PIT_BASE_FREQ / hz;

    outb(PIT_CTRL, 0x34);
    outb(PIT_CNT0, (uint8_t)(divisor & 0xFF));         
    outb(PIT_CNT0, (uint8_t)((divisor >> 8) & 0xFF));
}
