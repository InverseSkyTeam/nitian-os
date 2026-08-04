#include "interrupt.h"
#include "../../include/asm/stub.h"
#include "../../include/asmFunc.h"
#include "../io/io.h"
#include "../pic/pic.h"
#include "../pit/pit.h"

volatile uint32_t g_tick = 0;

/* CPU 异常名 */
static const char* g_exc_names[32] = {
    "Divide Error",             /* 0  #DE */
    "Debug",                    /* 1  #DB */
    "Non-Maskable Interrupt",   /* 2  NMI */
    "Breakpoint",               /* 3  #BP */
    "Overflow",                 /* 4  #OF */
    "BOUND Range Exceeded",     /* 5  #BR */
    "Invalid Opcode",           /* 6  #UD */
    "Device Not Available",     /* 7  #NM */
    "Double Fault",             /* 8  #DF */
    "Coprocessor Seg Overrun",  /* 9  (保留) */
    "Invalid TSS",              /* 10 #TS */
    "Segment Not Present",      /* 11 #NP */
    "Stack-Segment Fault",      /* 12 #SS */
    "General Protection",       /* 13 #GP */
    "Page Fault",               /* 14 #PF */
    "(Reserved)",               /* 15 */
    "x87 FP Error",             /* 16 #MF */
    "Alignment Check",          /* 17 #AC */
    "Machine Check",            /* 18 #MC */
    "SIMD FP Exception",        /* 19 #XF */
    "Virtualization Exception", /* 20 #VE */
    "Control Protection",       /* 21 #CP */
    "(Reserved)",               /* 22 */
    "(Reserved)",               /* 23 */
    "(Reserved)",               /* 24 */
    "(Reserved)",               /* 25 */
    "(Reserved)",               /* 26 */
    "(Reserved)",               /* 27 */
    "(Reserved)",               /* 28 */
    "(Reserved)",               /* 29 */
    "(Reserved)",               /* 30 */
    "(Reserved)"                /* 31 */
};

#define INT_NO_UNREGISTERED 0xFFFFu

void isr_handler(struct Registers* r) {
    uint32_t n = r->int_no;

    setTextColor(12);
    printf("\n*** EXCEPTION ***\n");

    if (n < 32) {
        printf("  %s (vector %d)\n", g_exc_names[n], (int)n);
        printf("  err_code = 0x%x\n", r->err_code);
    } else {
        printf("  Unregistered interrupt (vector %d)\n", (int)n);
    }

    printf("  eip = 0x%x  cs = 0x%x  eflags = 0x%x\n", r->eip, r->cs, r->eflags);
    printf("  eax = 0x%x  ebx = 0x%x  ecx = 0x%x  edx = 0x%x\n",
           r->eax, r->ebx, r->ecx, r->edx);
    printf("  esi = 0x%x  edi = 0x%x  ebp = 0x%x  ds = 0x%x\n",
           r->esi, r->edi, r->ebp, r->ds);

    asm_cli();
    for (;;) {
        asm_hlt();
    }
}

void irq_handler(struct Registers* r) {
    uint32_t irq = r->int_no - 32;

    if (irq == 0) {
        g_tick++;
        if (g_tick % PIT_HZ == 0) {    
            setTextColor(10); 
            printf("tick: %d\n", (int)g_tick);
        }
    }

    if (irq >= 8) {
        outb(PIC2_CMD, 0x20);
    }
    outb(PIC1_CMD, 0x20);
}
