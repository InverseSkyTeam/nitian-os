// 参考: 《操作系统真相还原》(于渊) 第8章 内存管理
#ifndef POOL_H
#define POOL_H

#include <stdint.h>
#include "../bitmap/bitmap.h"
#include "../paging/paging.h"

#define MEMORY_BASE 0x100000
#define MAX_PHYS_MEM 0x20000000

struct pool {
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
};

extern struct pool kernel_pool;

void mm_init(void);
void* palloc(struct pool* pool);
void pfree(struct pool* pool, uint32_t phy_addr);

#endif
