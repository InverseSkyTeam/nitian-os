#include <stdint.h>
#include "../../include/asmFunc.h"
#include "./pic.h"

int initPic(void) {
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    outb(PIC1_CMD, ICW1_INIT_NEED_ICW4);
    outb(PIC2_CMD, ICW1_INIT_NEED_ICW4);

    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    uint8_t imr1 = inb(PIC1_DATA);
    uint8_t imr2 = inb(PIC2_DATA);
    if (imr1 == 0xFF && imr2 == 0xFF) {
        outb(PIC1_DATA, 0xF8);
        outb(PIC2_DATA, 0xBF);
        return 0;
    }
    return -1;
}
