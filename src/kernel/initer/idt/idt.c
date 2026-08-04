#include "idt.h"
#include "../../include/asm/stub.h"

struct IDTEntry idt[256];

struct IDTR {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr0;

static void setIDT(uint8_t vec, void (*handler)(void), uint8_t type) {
    uint32_t addr = (uint32_t)handler;

    idt[vec].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[vec].selector    = 0x10;
    idt[vec].ist         = 0;
    idt[vec].type        = type;
    idt[vec].offset_high = (uint16_t)((addr >> 16) & 0xFFFF);
}

void initIDT(void) {
    for (int i = 0; i < 256; ++i) {
        setIDT((uint8_t)i, default_handler, IDT_TYPE_INT_GATE32);
    }

    setIDT(0,  isr0,  IDT_TYPE_INT_GATE32);
    setIDT(1,  isr1,  IDT_TYPE_INT_GATE32);
    setIDT(2,  isr2,  IDT_TYPE_INT_GATE32);
    setIDT(3,  isr3,  IDT_TYPE_INT_GATE32);
    setIDT(4,  isr4,  IDT_TYPE_INT_GATE32);
    setIDT(5,  isr5,  IDT_TYPE_INT_GATE32);
    setIDT(6,  isr6,  IDT_TYPE_INT_GATE32);
    setIDT(7,  isr7,  IDT_TYPE_INT_GATE32);
    setIDT(8,  isr8,  IDT_TYPE_INT_GATE32);
    setIDT(9,  isr9,  IDT_TYPE_INT_GATE32);
    setIDT(10, isr10, IDT_TYPE_INT_GATE32);
    setIDT(11, isr11, IDT_TYPE_INT_GATE32);
    setIDT(12, isr12, IDT_TYPE_INT_GATE32);
    setIDT(13, isr13, IDT_TYPE_INT_GATE32);
    setIDT(14, isr14, IDT_TYPE_INT_GATE32);
    setIDT(15, isr15, IDT_TYPE_INT_GATE32);
    setIDT(16, isr16, IDT_TYPE_INT_GATE32);
    setIDT(17, isr17, IDT_TYPE_INT_GATE32);
    setIDT(18, isr18, IDT_TYPE_INT_GATE32);
    setIDT(19, isr19, IDT_TYPE_INT_GATE32);
    setIDT(20, isr20, IDT_TYPE_INT_GATE32);
    setIDT(21, isr21, IDT_TYPE_INT_GATE32);
    setIDT(22, isr22, IDT_TYPE_INT_GATE32);
    setIDT(23, isr23, IDT_TYPE_INT_GATE32);
    setIDT(24, isr24, IDT_TYPE_INT_GATE32);
    setIDT(25, isr25, IDT_TYPE_INT_GATE32);
    setIDT(26, isr26, IDT_TYPE_INT_GATE32);
    setIDT(27, isr27, IDT_TYPE_INT_GATE32);
    setIDT(28, isr28, IDT_TYPE_INT_GATE32);
    setIDT(29, isr29, IDT_TYPE_INT_GATE32);
    setIDT(30, isr30, IDT_TYPE_INT_GATE32);
    setIDT(31, isr31, IDT_TYPE_INT_GATE32);

    setIDT(32, irq0,  IDT_TYPE_INT_GATE32);
    setIDT(33, irq1,  IDT_TYPE_INT_GATE32);
    setIDT(34, irq2,  IDT_TYPE_INT_GATE32);
    setIDT(35, irq3,  IDT_TYPE_INT_GATE32);
    setIDT(36, irq4,  IDT_TYPE_INT_GATE32);
    setIDT(37, irq5,  IDT_TYPE_INT_GATE32);
    setIDT(38, irq6,  IDT_TYPE_INT_GATE32);
    setIDT(39, irq7,  IDT_TYPE_INT_GATE32);
    setIDT(40, irq8,  IDT_TYPE_INT_GATE32);
    setIDT(41, irq9,  IDT_TYPE_INT_GATE32);
    setIDT(42, irq10, IDT_TYPE_INT_GATE32);
    setIDT(43, irq11, IDT_TYPE_INT_GATE32);
    setIDT(44, irq12, IDT_TYPE_INT_GATE32);
    setIDT(45, irq13, IDT_TYPE_INT_GATE32);
    setIDT(46, irq14, IDT_TYPE_INT_GATE32);
    setIDT(47, irq15, IDT_TYPE_INT_GATE32);

    setIDT(0x80, default_handler, IDT_TYPE_TRAP_GATE3);

    idtr0.limit = (uint16_t)(sizeof(idt) - 1);
    idtr0.base  = (uint32_t)idt;
    __asm__ volatile("lidt %0" : : "m"(idtr0) : "memory");
}
