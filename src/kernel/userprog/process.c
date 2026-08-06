// 参考: 《操作系统真相还原》(于渊) 第11章 用户进程
#include "./process.h"
#include "../initer/gdt/gdt.h"
#include "../initer/tss/tss.h"
#include "../include/asm/stub.h"
#include "../include/asmFunc.h"
#include "../memory/pool/pool.h"
#include "../memory/bitmap/bitmap.h"
#include "../lib/str/str.h"
#include "../include/assert.h"

#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / STEP)
#define EFLAGS_MBS (1 << 1)
#define EFLAGS_IF_1 (1 << 9)
#define EFLAGS_IOPL_0 0

void start_process(void* filename_) {
    void* function = filename_;
    struct task_struct* cur = current_task;
    uint32_t stack_top = cur->kernel_stack_top;
    uint32_t user_stack = (uint32_t)get_a_page(USER_STACK3_VADDR) + PAGE_SIZE;
    struct Registers* ps = (struct Registers*)(stack_top - THREAD_STACK_SIZE + 0x100);
    ps->edi = 0;
    ps->esi = 0;
    ps->ebp = 0;
    ps->esp = 0;
    ps->ebx = 0;
    ps->edx = 0;
    ps->ecx = 0;
    ps->eax = 0;
    ps->gs = SELECTOR_U_DATA;
    ps->fs = SELECTOR_U_DATA;
    ps->es = SELECTOR_U_DATA;
    ps->ds = SELECTOR_U_DATA;
    ps->int_no = 0;
    ps->err_code = 0;
    ps->eip = (uint32_t)function;
    ps->cs = SELECTOR_U_CODE;
    ps->eflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1;
    ps->user_esp = user_stack;
    ps->ss = SELECTOR_U_DATA;
    __asm__ volatile("movl %0, %%esp; jmp intr_exit" : : "g"(ps) : "memory");
}

void page_dir_activate(struct task_struct* pthread) {
    uint32_t page_dir_phy = 0x400000;
    if (pthread->pgdir != 0) {
        page_dir_phy = pthread->pgdir;
    }
    asm_write_cr3(page_dir_phy);
}

void process_activate(struct task_struct* pthread) {
    page_dir_activate(pthread);
    if (pthread->pgdir != 0) {
        update_tss_esp(pthread);
    }
}

uint32_t* create_page_dir(void) {
    uint32_t* page_dir_vaddr = (uint32_t*)palloc(&kernel_pool);
    if (page_dir_vaddr == 0) {
        return 0;
    }
    memset(page_dir_vaddr, 0, PAGE_SIZE);

    for (int i = 0; i < 64; i++) {
        page_dir_vaddr[i] = (uint32_t)(i * 0x400000) | 0x87;
    }

    memcpy(page_dir_vaddr + 768, (void*)0x400C00, 256 * 4);
    page_dir_vaddr[1023] = (uint32_t)page_dir_vaddr | 7;
    return page_dir_vaddr;
}

void create_user_vaddr_bitmap(struct task_struct* user_prog) {
    user_prog->userprog_v_addr.vaddr_start = USER_VADDR_START;
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8, PAGE_SIZE);
    user_prog->userprog_v_addr.vaddr_bitmap.bits = (uint8_t*)get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_v_addr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8;
    bitmap_init(&user_prog->userprog_v_addr.vaddr_bitmap);
}

void process_execute(void* filename, char* name) {

    struct task_struct* thread = thread_alloc_slot(name, DEFAULT_PRIO);

    struct thread_stack* ts = (struct thread_stack*)(thread->kernel_stack_top - sizeof(struct thread_stack));
    ts->eip = (void (*)(void))start_process;
    ts->unused_retaddr = 0;
    ts->function = filename;
    ts->func_arg = 0;

    create_user_vaddr_bitmap(thread);
    thread->pgdir = (uint32_t)create_page_dir();

    thread_ready(thread);
}
