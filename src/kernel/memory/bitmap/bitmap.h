// 参考: 《操作系统真相还原》(于渊) 第5章 位图内存管理
#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

#define BITMAP_MASK 0x80

struct bitmap {
    uint32_t btmp_bytes_len;
    uint8_t* bits;
};

void bitmap_init(struct bitmap* btmp);
int bitmap_scan_test(const struct bitmap* btmp, uint32_t bit_idx);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value);
int bitmap_scan(const struct bitmap* btmp, uint32_t cnt);

#endif
