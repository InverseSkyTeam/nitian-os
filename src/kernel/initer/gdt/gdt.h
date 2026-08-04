// 参考: 《操作系统真相还原》(于渊) 第11章 用户进程
#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define SELECTOR_KERNEL_DATA 0x08
#define SELECTOR_KERNEL_CODE 0x10
#define SELECTOR_TSS         0x18
#define SELECTOR_U_CODE      0x23
#define SELECTOR_U_DATA      0x2B

struct gdt_desc {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  attr_low;
    uint8_t  limit_high_attr_high;
    uint8_t  base_high;
} __attribute__((packed));

void gdt_init(void);
void set_tss_desc(uint32_t tss_base, uint32_t tss_limit);

#endif
