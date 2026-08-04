// 参考: 《操作系统真相还原》(于渊) 第11章 输入输出系统
#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>
#include "../lib/list/list.h"

struct semaphore {
    uint8_t value;
    struct list waiters;
};

struct lock {
    struct task_struct* holder;
    struct semaphore semaphore;
    uint32_t holder_repeat_nr;
};

void sema_init(struct semaphore* psema, uint8_t value);
void sema_down(struct semaphore* psema);
void sema_up(struct semaphore* psema);
void lock_init(struct lock* plock);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);

#endif
