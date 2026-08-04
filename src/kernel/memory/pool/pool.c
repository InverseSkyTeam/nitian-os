// 参考: 《操作系统真相还原》(于渊) 第8章 内存管理
#include "./pool.h"
#include "../../lib/str/str.h"

static uint8_t kernel_pool_bitmap[(MAX_PHYS_MEM - MEMORY_BASE) / PAGE_SIZE / 8];
struct pool kernel_pool;

static uint32_t e820_mem_upper(void) {
    uint32_t count = *(uint32_t*)0x6000;
    uint8_t* p = (uint8_t*)0x6004;
    uint32_t upper = 0;
    uint32_t i;
    for (i = 0; i < count; i++) {
        uint64_t base = *(uint64_t*)p;
        uint64_t len = *(uint64_t*)(p + 8);
        uint32_t type = *(uint32_t*)(p + 16);
        if (type == 1 && (uint32_t)(base + len) > upper) {
            upper = (uint32_t)(base + len);
        }
        p += 24;
    }
    return upper;
}

static void mark_used(uint32_t start, uint32_t size) {
    uint32_t end = start + size;
    while (start < end) {
        uint32_t idx = (start - kernel_pool.phy_addr_start) / PAGE_SIZE;
        if (idx < kernel_pool.pool_bitmap.btmp_bytes_len * 8) {
            bitmap_set(&kernel_pool.pool_bitmap, idx, 1);
        }
        start += PAGE_SIZE;
    }
}

void mm_init(void) {
    uint32_t upper = e820_mem_upper();
    kernel_pool.phy_addr_start = MEMORY_BASE;
    if (upper <= MEMORY_BASE) {
        upper = MEMORY_BASE + 0x100000;
    }
    if (upper > MAX_PHYS_MEM) {
        upper = MAX_PHYS_MEM;
    }
    kernel_pool.pool_size = upper - MEMORY_BASE;
    kernel_pool.pool_bitmap.bits = kernel_pool_bitmap;
    kernel_pool.pool_bitmap.btmp_bytes_len = sizeof(kernel_pool_bitmap);
    bitmap_init(&kernel_pool.pool_bitmap);
    mark_used(0x280000, 0x400000 - 0x280000);
    mark_used(0x400000, 0x450000 - 0x400000);
}

void* palloc(struct pool* pool) {
    int idx = bitmap_scan(&pool->pool_bitmap, 1);
    if (idx == -1) {
        return 0;
    }
    bitmap_set(&pool->pool_bitmap, (uint32_t)idx, 1);
    uint32_t addr = pool->phy_addr_start + (uint32_t)idx * PAGE_SIZE;
    memset((void*)addr, 0, PAGE_SIZE);
    return (void*)addr;
}

void pfree(struct pool* pool, uint32_t phy_addr) {
    if (phy_addr < pool->phy_addr_start) {
        return;
    }
    uint32_t idx = (phy_addr - pool->phy_addr_start) / PAGE_SIZE;
    bitmap_set(&pool->pool_bitmap, idx, 0);
}
