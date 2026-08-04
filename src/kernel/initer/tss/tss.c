// 参考: 《操作系统真相还原》(于渊) 第11章 用户进程
#include "tss.h"
#include "../gdt/gdt.h"
#include "../../thread/thread.h"
#include "../../memory/pool/pool.h"
#include "../../include/asmFunc.h"

struct tss tss;

void update_tss_esp(struct task_struct* pthread) {
    tss.esp0 = pthread->kernel_stack_top;
}

void tss_init(void) {
    uint32_t kstack = (uint32_t)palloc(&kernel_pool);

    tss.backlink = 0;
    tss.esp0 = kstack + PAGE_SIZE;
    tss.ss0 = SELECTOR_KERNEL_DATA;
    tss.esp1 = 0;
    tss.ss1 = 0;
    tss.esp2 = 0;
    tss.ss2 = 0;
    tss.cr3 = 0;
    tss.eip = 0;
    tss.eflags = 0;
    tss.eax = 0;
    tss.ecx = 0;
    tss.edx = 0;
    tss.ebx = 0;
    tss.esp = 0;
    tss.ebp = 0;
    tss.esi = 0;
    tss.edi = 0;
    tss.es = SELECTOR_KERNEL_DATA;
    tss.cs = SELECTOR_KERNEL_DATA;
    tss.ss = SELECTOR_KERNEL_DATA;
    tss.ds = SELECTOR_KERNEL_DATA;
    tss.fs = SELECTOR_KERNEL_DATA;
    tss.gs = SELECTOR_KERNEL_DATA;
    tss.ldt = 0;
    tss.trap = 0;
    tss.iomap_base = 0;

    set_tss_desc((uint32_t)&tss, sizeof(tss) - 1);
    asm_ltr(SELECTOR_TSS);
}
