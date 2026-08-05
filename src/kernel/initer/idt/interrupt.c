#include "interrupt.h"
#include "../../include/asm/stub.h"
#include "../../include/asmFunc.h"
#include "../io/io.h"
#include "../pic/pic.h"
#include "../pit/pit.h"
#include "../../device/keyboard.h"
#include "../../device/ide.h"
#include "../../thread/thread.h"

volatile uint32_t g_tick = 0;

static const char* g_exc_names[32] = {
    "Divide Error",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Seg Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection",
    "Page Fault",
    "(Reserved)",
    "x87 FP Error",
    "Alignment Check",
    "Machine Check",
    "SIMD FP Exception",
    "Virtualization Exception",
    "Control Protection",
    "(Reserved)",
    "(Reserved)",
    "(Reserved)",
    "(Reserved)",
    "(Reserved)",
    "(Reserved)",
    "(Reserved)",
    "(Reserved)",
    "(Reserved)",
    "(Reserved)"
};

#define INT_NO_UNREGISTERED 0xFFFFu

void isr_handler(struct Registers* r) {
    uint32_t n = r->int_no;

    setTextColor(12);
    kprintf("\n*** EXCEPTION ***\n");

    if (n < 32) {
        kprintf("  %s (vector %d)\n", g_exc_names[n], (int)n);
        kprintf("  err_code = 0x%x\n", r->err_code);
    } else {
        kprintf("  Unregistered interrupt (vector %d)\n", (int)n);
    }

    kprintf("  eip = 0x%x  cs = 0x%x  eflags = 0x%x\n", r->eip, r->cs, r->eflags);
    kprintf("  eax = 0x%x  ebx = 0x%x  ecx = 0x%x  edx = 0x%x\n",
           r->eax, r->ebx, r->ecx, r->edx);
    kprintf("  esi = 0x%x  edi = 0x%x  ebp = 0x%x  ds = 0x%x\n",
           r->esi, r->edi, r->ebp, r->ds);

    asm_cli();
    for (;;) {
        asm_hlt();
    }
}

void irq_handler(struct Registers* r) {
    uint32_t irq = r->int_no - 32;

    if (irq >= 8) {
        outb(PIC2_CMD, 0x20);
    }
    outb(PIC1_CMD, 0x20);

    if (irq == 0) {
        g_tick++;
        if (g_tick % PIT_HZ == 0) {
            setTextColor(10);
            kprintf("tick: %d\n", (int)g_tick);
        }
        if (current_task != 0) {
            schedule();
        }
    }

    if (irq == 1) {
        keyboard_handler();
    }

    if (irq == 14) {
        intr_hd_handler((uint8_t)r->int_no);
    }
}
