// 参考: 《操作系统真相还原》(于渊) 第5章 内存分页
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_SIZE 0x1000
#define IDENTITY_MAP_END 0x10000000

void init_paging(uint32_t vram_addr);

#endif
