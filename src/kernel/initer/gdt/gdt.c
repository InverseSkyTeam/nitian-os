// 参考: 《操作系统真相还原》(于渊) 第11章 用户进程
#include "gdt.h"
#include "../../include/asmFunc.h"

struct gdt_desc gdt[6];

struct gdtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdtr0;

static void desc_init(struct gdt_desc* d, uint32_t base, uint32_t limit,
                      uint8_t attr_low, uint8_t attr_high) {
    d->limit_low = limit & 0xFFFF;
    d->base_low = base & 0xFFFF;
    d->base_mid = (base >> 16) & 0xFF;
    d->attr_low = attr_low;
    d->limit_high_attr_high = ((limit >> 16) & 0x0F) | attr_high;
    d->base_high = (base >> 24) & 0xFF;
}

void set_tss_desc(uint32_t tss_base, uint32_t tss_limit) {
    desc_init(&gdt[3], tss_base, tss_limit, 0x89, 0x00);
}

void gdt_init(void) {
    desc_init(&gdt[0], 0, 0, 0, 0);
    desc_init(&gdt[1], 0, 0xFFFFF, 0x92, 0xCF);
    desc_init(&gdt[2], 0, 0xFFFFF, 0x9A, 0xCF);
    desc_init(&gdt[4], 0, 0xFFFFF, 0xFA, 0xCF);
    desc_init(&gdt[5], 0, 0xFFFFF, 0xF2, 0xCF);

    gdtr0.limit = (uint16_t)(sizeof(gdt) - 1);
    gdtr0.base = (uint32_t)gdt;
    asm_lgdt((uint32_t)&gdtr0);
}
