// 参考: 《操作系统真相还原》(于渊) 第9章 线程与调度
#include "./thread.h"
#include "../lib/str/str.h"
#include "../lib/list/list.h"
#include "../memory/pool/pool.h"
#include "../userprog/process.h"
#include "../device/keyboard.h"
#include "../include/asmFunc.h"
#include "../include/assert.h"

static struct list g_ready_list;
static struct task_struct g_task_table[MAX_TASKS];
static uint32_t g_task_count = 0;
static uint32_t g_pid_alloc = 0;
struct list g_thread_all_list;

struct task_struct* current_task;
struct task_struct* idle_thread;

uint32_t g_foreground_pid = (uint32_t)-1;
uint32_t g_init_pid = 1;

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

static void init_fd_table(struct task_struct* t) {
    t->fd_table[0] = 0;
    t->fd_table[1] = 1;
    t->fd_table[2] = 2;
    uint32_t fd_idx = 3;
    while (fd_idx < MAX_FILES_OPEN_PER_PROC) {
        t->fd_table[fd_idx++] = (uint32_t)-1;
    }
    t->cwd_inode_nr = 0;
}

static void init_task_struct_basic(struct task_struct* t, int32_t parent_pid) {
    t->status = TASK_READY;
    t->pid = g_pid_alloc++;
    t->elapsed_ticks = 0;
    t->kernel_stack_top = 0;
    t->pgdir = 0;
    init_fd_table(t);
    t->parent_pid = parent_pid;
    t->stack_magic = STACK_MAGIC;

    t->general_tag.prev = t->general_tag.next = NULL;
    t->all_list_tag.prev = t->all_list_tag.next = NULL;
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
    init_task_struct_basic(t, -1);
    strcpy(t->name, name);
    t->priority = priority;
    t->ticks = priority;
    t->kernel_stack_top = stack + THREAD_STACK_SIZE;
    list_append(&g_ready_list, &t->general_tag);
    list_append(&g_thread_all_list, &t->all_list_tag);
    return t;
}

void thread_init(void) {
    list_init(&g_ready_list);
    list_init(&g_thread_all_list);
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
    init_fd_table(&g_task_table[0]);
    g_task_table[0].parent_pid = -1;
    g_task_table[0].stack_magic = STACK_MAGIC;

    list_append(&g_thread_all_list, &g_task_table[0].all_list_tag);
    g_task_table[0].general_tag.prev = g_task_table[0].general_tag.next = NULL;
    g_task_count = 1;

    idle_thread = thread_create("idle", 10, idle, 0);
}

struct task_struct* thread_alloc_slot(const char* name, uint8_t priority) {
    if (g_task_count >= MAX_TASKS) {
        ASSERT(0 && "no task slot");
        return NULL;
    }
    struct task_struct* t = &g_task_table[g_task_count++];
    uint32_t stack = (uint32_t)get_kernel_pages(THREAD_STACK_SIZE / PAGE_SIZE);
    struct thread_stack* ts = (struct thread_stack*)(stack + THREAD_STACK_SIZE - sizeof(struct thread_stack));
    ts->eflags = 0x202;
    ts->esi = ts->edi = ts->ebx = ts->ebp = 0;
    ts->eip = 0;              
    ts->unused_retaddr = 0;
    ts->function = 0;
    ts->func_arg = 0;
    t->self_kstack = (uint32_t*)ts;
    init_task_struct_basic(t, -1);
    strcpy(t->name, name);
    t->priority = priority;
    t->ticks = priority;
    t->kernel_stack_top = stack + THREAD_STACK_SIZE;
    list_append(&g_thread_all_list, &t->all_list_tag);
    return t;
}

void thread_ready(struct task_struct* t) {
    if (t == NULL) return;
    uint32_t old = asm_save_eflags();
    asm_cli();

    if (!elem_find(&g_ready_list, &t->general_tag)) {
        t->status = TASK_READY;
        list_append(&g_ready_list, &t->general_tag);
    }
    asm_restore_eflags(old);
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

void thread_block_with_status(enum task_status status) {
    uint32_t old = asm_save_eflags();
    asm_cli();
    current_task->status = status;
    schedule();
    asm_restore_eflags(old);
}

void thread_unblock(struct task_struct* t) {
    uint32_t old = asm_save_eflags();
    asm_cli();
    ASSERT(t->status == TASK_BLOCKED || t->status == TASK_WAITING ||
           t->status == TASK_HANGING);
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

struct fork_args {
    fork_continuation cb;
    void* user_arg;
    uint32_t child_pid;
};

static void fork_thread_entry(void* arg_) {
    struct fork_args* fa = (struct fork_args*)arg_;
    fa->cb(fa->user_arg, fa->child_pid, 1);  
}

static void build_fork_thread_stack(struct task_struct* t, struct fork_args* fa) {
    struct thread_stack* ts = (struct thread_stack*)(t->kernel_stack_top - sizeof(struct thread_stack));
    ts->eflags = 0x202;
    ts->esi = ts->edi = ts->ebx = ts->ebp = 0;
    ts->eip = (void (*)(void))kernel_thread_entry;
    ts->unused_retaddr = 0;
    ts->function = fork_thread_entry;
    ts->func_arg = fa;
    t->self_kstack = (uint32_t*)ts;
}

int thread_fork_with_cb(const char* name, uint8_t priority,
                        fork_continuation cb, void* arg) {
    if (cb == NULL) return -1;
    uint32_t old = asm_save_eflags();
    asm_cli();

    struct task_struct* parent = current_task;
    struct task_struct* child = thread_alloc_slot(name, priority);
    if (child == NULL) {
        asm_restore_eflags(old);
        return -1;
    }
    child->parent_pid = (int32_t)parent->pid;

    size_t nlen = strnlen(name, 15);
    if (nlen + 5 < 16) {
        memcpy(child->name, name, nlen);
        memcpy(child->name + nlen, "_fork", 5);
        child->name[nlen + 5] = 0;
    }
    child->priority = priority;
    child->ticks = priority;

    child->cwd_inode_nr = parent->cwd_inode_nr;
    for (uint32_t i = 0; i < MAX_FILES_OPEN_PER_PROC; i++) {
        child->fd_table[i] = parent->fd_table[i];
    }

    struct fork_args* fa = (struct fork_args*)get_kernel_pages(1);
    if (fa == NULL) {
        asm_restore_eflags(old);
        return -1;
    }
    fa->cb = cb;
    fa->user_arg = arg;
    fa->child_pid = child->pid;

    build_fork_thread_stack(child, fa);
    thread_ready(child);

    asm_restore_eflags(old);
    return (int)child->pid;
}

int thread_traverse_all(thread_all_action action, void* arg) {
    int stopped = 0;
    struct list_elem* e = g_thread_all_list.head.next;
    while (e != &g_thread_all_list.tail) {
        struct task_struct* t = list_entry(e, struct task_struct, all_list_tag);
        struct list_elem* next = e->next;
        int r = action(t, arg);
        if (r) { stopped = 1; break; }
        e = next;
    }
    return stopped;
}

void thread_exit_current(void) {
    uint32_t old = asm_save_eflags();
    asm_cli();
    current_task->status = TASK_DIED;
    if (elem_find(&g_ready_list, &current_task->general_tag)) {
        list_remove(&current_task->general_tag);
    }
    schedule();
    asm_restore_eflags(old);
}

void thread_kill_pid(uint32_t pid) {
    struct task_struct* t = NULL;
    for (uint32_t i = 0; i < g_task_count; i++) {
        if (g_task_table[i].pid == pid) {
            t = &g_task_table[i];
            break;
        }
    }
    if (t == NULL || t->status == TASK_DIED || t->status == TASK_HANGING) return;
    if (t->pgdir == 0) return;                              

    uint32_t old = asm_save_eflags();
    asm_cli();
    t->exit_status = -1;                                     
    t->status = TASK_HANGING;                                    

    if (elem_find(&g_ready_list, &t->general_tag)) {
        list_remove(&t->general_tag);
    }

    for (uint32_t i = 0; i < g_task_count; i++) {
        if (g_task_table[i].parent_pid == (int32_t)t->pid) {
            g_task_table[i].parent_pid = (int32_t)g_init_pid;
        }
    }

    if (keyboard_ioq.consumer == t) keyboard_ioq.consumer = 0;
    if (keyboard_ioq.producer == t) keyboard_ioq.producer = 0;

    struct task_struct* parent = pid2thread(t->parent_pid);
    if (parent && parent->status == TASK_WAITING) {
        thread_unblock(parent);
    }
    if (t == current_task) {
        schedule();                                              
    }
    asm_restore_eflags(old);
}

int thread_is_died(uint32_t pid) {
    for (uint32_t i = 0; i < g_task_count; i++) {
        if (g_task_table[i].pid == pid) {
            return (g_task_table[i].status == TASK_DIED ||
                    g_task_table[i].status == TASK_HANGING);
        }
    }
    return 1;                                          
}

struct task_struct* pid2thread(int32_t pid) {
    for (uint32_t i = 0; i < g_task_count; i++) {
        if ((int32_t)g_task_table[i].pid == pid) {
            return &g_task_table[i];
        }
    }
    return NULL;
}

void thread_exit(struct task_struct* thread_over, int need_schedule) {
    uint32_t old = asm_save_eflags();
    asm_cli();
    thread_over->status = TASK_DIED;
    if (elem_find(&g_ready_list, &thread_over->general_tag)) {
        list_remove(&thread_over->general_tag);
    }
    if (thread_over->pgdir) {
        pfree(&kernel_pool, thread_over->pgdir);
        thread_over->pgdir = 0;
    }
    if (elem_find(&g_thread_all_list, &thread_over->all_list_tag)) {
        list_remove(&thread_over->all_list_tag);
    }
    asm_restore_eflags(old);
    if (need_schedule) {
        schedule();
    }
}
