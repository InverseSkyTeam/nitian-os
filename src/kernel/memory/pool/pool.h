// 参考: 《操作系统真相还原》(于渊) 第8章 内存管理
#ifndef POOL_H
#define POOL_H

#include <stdint.h>
#include "../bitmap/bitmap.h"

#define PAGE_SIZE 0x1000
#define MEMORY_BASE 0x100000
#define MAX_PHYS_MEM 0x20000000

struct pool {
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
};

struct virtual_addr {
    struct bitmap vaddr_bitmap;
    uint32_t vaddr_start;
};

extern struct pool kernel_pool;
extern struct virtual_addr kernel_vaddr;

void mm_init(void);
void* palloc(struct pool* pool);
void pfree(struct pool* pool, uint32_t phy_addr);
uint32_t palloc_pages(struct pool* pool, uint32_t cnt);

uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
void page_table_add(uint32_t vaddr, uint32_t phy_addr);
void* get_a_page(uint32_t vaddr);
void* get_kernel_pages(uint32_t pg_cnt);
void free_kernel_page(uint32_t vaddr);

#endif
