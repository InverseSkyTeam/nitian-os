// 参考: 《操作系统真相还原》(于渊) 第11章 输入输出系统
#include "./sync.h"
#include "./thread.h"
#include "../include/asmFunc.h"
#include "../include/assert.h"

void sema_init(struct semaphore* psema, uint8_t value) {
    psema->value = value;
    list_init(&psema->waiters);
}

void sema_down(struct semaphore* psema) {
    uint32_t old = asm_save_eflags();
    asm_cli();
    while (psema->value == 0) {
        list_append(&psema->waiters, &current_task->general_tag);
        thread_block();
    }
    psema->value--;
    ASSERT(psema->value == 0);
    asm_restore_eflags(old);
}

void sema_up(struct semaphore* psema) {
    uint32_t old = asm_save_eflags();
    asm_cli();
    ASSERT(psema->value == 0);
    if (!list_empty(&psema->waiters)) {
        struct list_elem* e = list_pop_front(&psema->waiters);
        struct task_struct* w = list_entry(e, struct task_struct, general_tag);
        thread_unblock(w);
    }
    psema->value++;
    ASSERT(psema->value == 1);
    asm_restore_eflags(old);
}

void lock_init(struct lock* plock) {
    plock->holder = 0;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1);
}

void lock_acquire(struct lock* plock) {
    if (plock->holder != current_task) {
        sema_down(&plock->semaphore);
        plock->holder = current_task;
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    } else {
        plock->holder_repeat_nr++;
    }
}

void lock_release(struct lock* plock) {
    ASSERT(plock->holder == current_task);
    if (plock->holder_repeat_nr > 1) {
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);
    plock->holder = 0;
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);
}
