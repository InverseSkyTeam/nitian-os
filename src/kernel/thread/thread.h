// 参考: 《操作系统真相还原》(于渊) 第9章 线程与调度
#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#include "../lib/list/list.h"
#include "../memory/pool/pool.h"

#define THREAD_STACK_SIZE 0x2000
#define MAX_TASKS 64
#define STACK_MAGIC 0x19860726
#define MAX_FILES_OPEN_PER_PROC 8

enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_DIED
};

typedef void (*thread_func)(void*);

struct thread_stack {
    uint32_t eflags;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebx;
    uint32_t ebp;
    void (*eip)(void);
    void (*unused_retaddr);
    thread_func function;
    void* func_arg;
};

struct task_struct {
    uint32_t* self_kstack;
    enum task_status status;
    uint32_t pid;
    char name[16];
    uint8_t priority;
    uint8_t ticks;
    uint32_t elapsed_ticks;
    struct list_elem general_tag;
    struct list_elem all_list_tag;
    int32_t parent_pid;              
    uint32_t kernel_stack_top;
    uint32_t pgdir;
    struct virtual_addr userprog_v_addr;
    uint32_t cwd_inode_nr;
    uint32_t fd_table[MAX_FILES_OPEN_PER_PROC];
    uint32_t stack_magic;
};

extern struct task_struct* current_task;
extern struct task_struct* idle_thread;
extern struct list g_thread_all_list;   

void thread_init(void);
void kernel_thread(char* name, uint8_t priority, thread_func function, void* arg);
struct task_struct* thread_create(char* name, uint8_t priority, thread_func function, void* arg);
void schedule(void);
void switch_to(uint32_t** cur_kstack, uint32_t** next_kstack);
void thread_block(void);
void thread_unblock(struct task_struct* t);
void thread_yield(void);

typedef int (*thread_all_action)(struct task_struct*, void*);
int thread_traverse_all(thread_all_action action, void* arg);

struct task_struct* thread_alloc_slot(const char* name, uint8_t priority);

void thread_ready(struct task_struct* t);

typedef void (*fork_continuation)(void* arg, uint32_t child_pid, int is_child);
int thread_fork_with_cb(const char* name, uint8_t priority,
                        fork_continuation cb, void* arg);

#endif
