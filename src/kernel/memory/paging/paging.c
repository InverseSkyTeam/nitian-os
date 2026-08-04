// 参考: 《操作系统真相还原》(于渊) 第5章 内存分页
#include "./paging.h"
#include "../../lib/str/str.h"
#include "../../include/asmFunc.h"

#define PDE_BASE 0x400000
#define PTE_BASE 0x401000
#define PTE_COUNT 1024
#define PDE_FLAGS 0x003
#define VRAM_PTE_SLOT 64

void init_paging(uint32_t vram_addr) {
    uint32_t i, j;
    uint32_t* pde = (uint32_t*)PDE_BASE;
    memset(pde, 0, PAGE_SIZE);

    for (i = 0; i * 0x400000 < IDENTITY_MAP_END; i++) {
        uint32_t* pte = (uint32_t*)(PTE_BASE + i * PAGE_SIZE);
        memset(pte, 0, PAGE_SIZE);
        pde[i] = (uint32_t)(PTE_BASE + i * PAGE_SIZE) | PDE_FLAGS;
        for (j = 0; j < PTE_COUNT; j++) {
            pte[j] = (i * 0x400000 + j * PAGE_SIZE) | PDE_FLAGS;
        }
    }

    pde[vram_addr >> 22] = (uint32_t)(PTE_BASE + VRAM_PTE_SLOT * PAGE_SIZE) | PDE_FLAGS;
    {
        uint32_t* vpt = (uint32_t*)(PTE_BASE + VRAM_PTE_SLOT * PAGE_SIZE);
        uint32_t base = vram_addr & 0xFFC00000;
        memset(vpt, 0, PAGE_SIZE);
        for (i = 0; i < PTE_COUNT; i++) {
            vpt[i] = (base + i * PAGE_SIZE) | PDE_FLAGS;
        }
    }

    asm_write_cr3(PDE_BASE);
    asm_write_cr0(asm_read_cr0() | 0x80000000);
}
