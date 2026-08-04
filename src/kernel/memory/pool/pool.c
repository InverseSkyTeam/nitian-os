// 参考: 《操作系统真相还原》(于渊) 第8章 内存管理
#include "./pool.h"
#include "../../lib/str/str.h"
#include "../../thread/thread.h"
#include "../../include/assert.h"

#define PDE_INDEX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_INDEX(addr) ((addr & 0x003ff000) >> 12)

static uint8_t kernel_pool_bitmap[(MAX_PHYS_MEM - MEMORY_BASE) / PAGE_SIZE / 8];
static uint8_t kernel_vaddr_bitmap[0x400000 / PAGE_SIZE / 8];
struct pool kernel_pool;
struct virtual_addr kernel_vaddr;

#define KERNEL_VADDR_START 0xC1000000

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
    mark_used(0x400000, 0x460000 - 0x400000);

    kernel_vaddr.vaddr_start = KERNEL_VADDR_START;
    kernel_vaddr.vaddr_bitmap.bits = kernel_vaddr_bitmap;
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = sizeof(kernel_vaddr_bitmap);
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
}

void* palloc(struct pool* pool) {
    int idx = bitmap_scan(&pool->pool_bitmap, 1);
    if (idx == -1) {
        return 0;
    }
    bitmap_set(&pool->pool_bitmap, (uint32_t)idx, 1);
    uint32_t addr = pool->phy_addr_start + (uint32_t)idx * PAGE_SIZE;
    return (void*)addr;
}

void pfree(struct pool* pool, uint32_t phy_addr) {
    if (phy_addr < pool->phy_addr_start) {
        return;
    }
    uint32_t idx = (phy_addr - pool->phy_addr_start) / PAGE_SIZE;
    bitmap_set(&pool->pool_bitmap, idx, 0);
}

uint32_t palloc_pages(struct pool* pool, uint32_t cnt) {
    int idx = bitmap_scan(&pool->pool_bitmap, cnt);
    if (idx == -1) {
        return 0;
    }
    for (uint32_t i = 0; i < cnt; i++) {
        bitmap_set(&pool->pool_bitmap, (uint32_t)idx + i, 1);
    }
    uint32_t addr = pool->phy_addr_start + (uint32_t)idx * PAGE_SIZE;
    return addr;
}

uint32_t* pde_ptr(uint32_t vaddr) {
    return (uint32_t*)(0xfffff000 + PDE_INDEX(vaddr) * 4);
}

uint32_t* pte_ptr(uint32_t vaddr) {
    return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_INDEX(vaddr) * 4);
}

void page_table_add(uint32_t vaddr, uint32_t phy_addr) {
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);

    if (*pde & 1) {
        ASSERT(!(*pte & 1));
        *pte = phy_addr | 7;
    } else {
        uint32_t pde_phy = (uint32_t)palloc(&kernel_pool);
        *pde = pde_phy | 7;
        memset((void*)((uint32_t)pte & 0xfffff000), 0, PAGE_SIZE);
        ASSERT(!(*pte & 1));
        *pte = phy_addr | 7;
    }
}

void* get_a_page(uint32_t vaddr) {
    struct task_struct* cur = current_task;
    uint32_t bit_idx = (vaddr - cur->userprog_v_addr.vaddr_start) / PAGE_SIZE;
    ASSERT(bit_idx < cur->userprog_v_addr.vaddr_bitmap.btmp_bytes_len * 8);
    bitmap_set(&cur->userprog_v_addr.vaddr_bitmap, bit_idx, 1);
    uint32_t phy = (uint32_t)palloc(&kernel_pool);
    if (phy == 0) {
        return 0;
    }
    page_table_add(vaddr, phy);
    memset((void*)vaddr, 0, PAGE_SIZE);
    return (void*)vaddr;
}

void* get_kernel_pages(uint32_t pg_cnt) {
    int bit = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
    if (bit == -1) {
        return 0;
    }
    uint32_t vaddr = kernel_vaddr.vaddr_start + (uint32_t)bit * PAGE_SIZE;
    for (uint32_t i = 0; i < pg_cnt; i++) {
        bitmap_set(&kernel_vaddr.vaddr_bitmap, (uint32_t)bit + i, 1);
        uint32_t phy = (uint32_t)palloc(&kernel_pool);
        page_table_add(vaddr + i * PAGE_SIZE, phy);
        memset((void*)(vaddr + i * PAGE_SIZE), 0, PAGE_SIZE);
    }
    return (void*)vaddr;
}
