// 参考: 《操作系统真相还原》(于渊) 第5章 位图内存管理
#include "./bitmap.h"
#include "../../lib/str/str.h"

void bitmap_init(struct bitmap* btmp) {
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

int bitmap_scan_test(const struct bitmap* btmp, uint32_t bit_idx) {
    uint32_t byte = bit_idx / 8;
    if (byte >= btmp->btmp_bytes_len) {
        return -1;
    }
    return (btmp->bits[byte] & (BITMAP_MASK >> (bit_idx % 8))) ? 1 : 0;
}

void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value) {
    uint32_t byte = bit_idx / 8;
    if (byte >= btmp->btmp_bytes_len) {
        return;
    }
    if (value) {
        btmp->bits[byte] |= (BITMAP_MASK >> (bit_idx % 8));
    } else {
        btmp->bits[byte] &= ~(BITMAP_MASK >> (bit_idx % 8));
    }
}

int bitmap_scan(const struct bitmap* btmp, uint32_t cnt) {
    uint32_t total = btmp->btmp_bytes_len * 8;
    uint32_t start = 0;
    if (cnt == 0 || total == 0) {
        return -1;
    }
    while (start < total) {
        while (start < total && bitmap_scan_test(btmp, start)) {
            start++;
        }
        uint32_t run = 0;
        uint32_t begin = start;
        while (start < total && !bitmap_scan_test(btmp, start) && run < cnt) {
            run++;
            start++;
        }
        if (run == cnt) {
            return (int)begin;
        }
    }
    return -1;
}
