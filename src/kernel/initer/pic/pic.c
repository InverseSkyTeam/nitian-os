/**
 * 参考: https://wiki.osdev.org/8259_PIC
 */

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

    // 在 PIC 初始化完成后尝试通过回读 IMR 验证 PIC 是否能够正常工作
    uint8_t imr1 = inb(PIC1_DATA);
    uint8_t imr2 = inb(PIC2_DATA);
    if (imr1 == 0xFF && imr2 == 0xFF) {
        outb(PIC1_DATA, 0xFC);
        outb(PIC2_DATA, 0xFF);
        return 0;
    }
    return -1;
}
