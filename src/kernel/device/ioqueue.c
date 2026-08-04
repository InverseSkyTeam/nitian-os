// 参考: 《操作系统真相还原》(于渊) 第11章 输入输出系统
#include "./ioqueue.h"
#include "../thread/sync.h"
#include "../include/asmFunc.h"
#include "../include/assert.h"

void ioq_init(struct ioqueue* ioq) {
    lock_init(&ioq->lock);
    ioq->producer = 0;
    ioq->consumer = 0;
    ioq->head = 0;
    ioq->tail = 0;
}

static int32_t next_pos(int32_t pos) {
    return (pos + 1) % BUFSIZE;
}

int ioq_full(struct ioqueue* ioq) {
    return next_pos(ioq->head) == ioq->tail;
}

int ioq_empty(struct ioqueue* ioq) {
    return ioq->head == ioq->tail;
}

static void ioq_wait(struct task_struct** waiter) {
    *waiter = current_task;
    thread_block();
}

static void wakeup(struct task_struct** waiter) {
    struct task_struct* w = *waiter;
    *waiter = 0;
    if (w) {
        thread_unblock(w);
    }
}

void ioq_putchar(struct ioqueue* ioq, char byte) {
    ASSERT((asm_save_eflags() & 0x200) == 0);

    while (ioq_full(ioq)) {
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->producer);
        lock_release(&ioq->lock);
    }
    ioq->buf[ioq->head] = byte;
    ioq->head = next_pos(ioq->head);

    if (ioq->consumer != 0) {
        wakeup(&ioq->consumer);
    }
}

char ioq_getchar(struct ioqueue* ioq) {
    ASSERT((asm_save_eflags() & 0x200) == 0);

    while (ioq_empty(ioq)) {
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }
    char byte = ioq->buf[ioq->tail];
    ioq->tail = next_pos(ioq->tail);

    if (ioq->producer != 0) {
        wakeup(&ioq->producer);
    }
    return byte;
}
