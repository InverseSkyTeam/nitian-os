// 参考: 《操作系统真相还原》(于渊) 第13章 硬盘驱动(休眠函数)
#include "pit.h"
#include "../../include/asmFunc.h"
#include "../idt/interrupt.h"
#include "../../thread/thread.h"

#define PIT_BASE_FREQ 1193182

#define MIL_SECOND_PER_INTR (1000 / PIT_HZ)

void initPIT(uint32_t hz) {
    uint32_t divisor = PIT_BASE_FREQ / hz;

    outb(PIT_CTRL, 0x34);
    outb(PIT_CNT0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CNT0, (uint8_t)((divisor >> 8) & 0xFF));
}

static void ticks_to_sleep(uint32_t sleep_ticks) {
    uint32_t start_tick = g_tick;
    while (g_tick - start_tick < sleep_ticks) {
        thread_yield();
    }
}

void mtime_sleep(uint32_t m_seconds) {
    uint32_t sleep_ticks = (m_seconds + MIL_SECOND_PER_INTR - 1) / MIL_SECOND_PER_INTR;
    if (sleep_ticks == 0) {
        sleep_ticks = 1;
    }
    ticks_to_sleep(sleep_ticks);
}
