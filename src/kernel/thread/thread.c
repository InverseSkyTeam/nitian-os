// 参考: 《操作系统真相还原》(于渊) 第9章 线程与调度
#include "./thread.h"
#include "../lib/str/str.h"
#include "../memory/pool/pool.h"
#include "../userprog/process.h"
#include "../include/asmFunc.h"
#include "../include/assert.h"

static struct list g_ready_list;
static struct task_struct g_task_table[MAX_TASKS];
static uint32_t g_task_count = 0;
static uint32_t g_pid_alloc = 0;

struct task_struct* current_task;
struct task_struct* idle_thread;

static void idle(void* arg) {
    for (;;) {
        thread_block();
        __asm__ volatile("sti; hlt" : : : "memory");
    }
}

static void kernel_thread_entry(thread_func function, void* arg) {
    function(arg);
    current_task->status = TASK_DIED;
    uint32_t old = asm_save_eflags();
    asm_cli();
    schedule();
    asm_restore_eflags(old);
    for (;;) {
        asm_hlt();
    }
}

struct task_struct* thread_create(char* name, uint8_t priority, thread_func function, void* arg) {
    struct task_struct* t = &g_task_table[g_task_count++];
    uint32_t stack = (uint32_t)get_kernel_pages(THREAD_STACK_SIZE / PAGE_SIZE);
    struct thread_stack* ts = (struct thread_stack*)(stack + THREAD_STACK_SIZE - sizeof(struct thread_stack));
    ts->eflags = 0x202;
    ts->esi = 0;
    ts->edi = 0;
    ts->ebx = 0;
    ts->ebp = 0;
    ts->eip = (void (*)(void))kernel_thread_entry;
    ts->unused_retaddr = 0;
    ts->function = function;
    ts->func_arg = arg;
    t->self_kstack = (uint32_t*)ts;
    t->status = TASK_READY;
    t->pid = g_pid_alloc++;
    strcpy(t->name, name);
    t->priority = priority;
    t->ticks = priority;
    t->elapsed_ticks = 0;
    t->kernel_stack_top = stack + THREAD_STACK_SIZE;
    t->pgdir = 0;
    t->stack_magic = STACK_MAGIC;
    list_append(&g_ready_list, &t->general_tag);
    return t;
}

void thread_init(void) {
    list_init(&g_ready_list);
    current_task = &g_task_table[0];
    g_task_table[0].self_kstack = 0;
    g_task_table[0].status = TASK_RUNNING;
    g_task_table[0].pid = g_pid_alloc++;
    strcpy(g_task_table[0].name, "main");
    g_task_table[0].priority = 5;
    g_task_table[0].ticks = 5;
    g_task_table[0].elapsed_ticks = 0;
    g_task_table[0].kernel_stack_top = 0;
    g_task_table[0].pgdir = 0;
    g_task_table[0].stack_magic = STACK_MAGIC;
    g_task_count = 1;

    idle_thread = thread_create("idle", 10, idle, 0);
}

void kernel_thread(char* name, uint8_t priority, thread_func function, void* arg) {
    thread_create(name, priority, function, arg);
}

void thread_block(void) {
    uint32_t old = asm_save_eflags();
    asm_cli();
    current_task->status = TASK_BLOCKED;
    schedule();
    asm_restore_eflags(old);
}

void thread_unblock(struct task_struct* t) {
    uint32_t old = asm_save_eflags();
    asm_cli();
    ASSERT(t->status == TASK_BLOCKED);
    if (t->status != TASK_READY) {
        ASSERT(!elem_find(&g_ready_list, &t->general_tag));
        list_push(&g_ready_list, &t->general_tag);
        t->status = TASK_READY;
    }
    asm_restore_eflags(old);
}

void thread_yield(void) {
    uint32_t old = asm_save_eflags();
    asm_cli();
    current_task->status = TASK_READY;
    list_append(&g_ready_list, &current_task->general_tag);
    current_task->ticks = current_task->priority;
    schedule();
    asm_restore_eflags(old);
}

void schedule(void) {
    ASSERT((asm_save_eflags() & 0x200) == 0);

    if (current_task->status == TASK_RUNNING) {
        current_task->status = TASK_READY;
        list_append(&g_ready_list, &current_task->general_tag);
        current_task->ticks = current_task->priority;
    }
    if (list_empty(&g_ready_list)) {
        thread_unblock(idle_thread);
    }
    struct list_elem* e = list_pop_front(&g_ready_list);
    struct task_struct* next = list_entry(e, struct task_struct, general_tag);
    next->status = TASK_RUNNING;
    struct task_struct* prev = current_task;
    current_task = next;
    process_activate(next);
    switch_to(&prev->self_kstack, &next->self_kstack);
}
