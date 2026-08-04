// 参考: 《操作系统真相还原》(于渊) 第11章 输入输出系统
#ifndef IOQUEUE_H
#define IOQUEUE_H

#include <stdint.h>
#include "../thread/sync.h"
#include "../thread/thread.h"

#define BUFSIZE 64

struct ioqueue {
    struct lock lock;
    struct task_struct* producer;
    struct task_struct* consumer;
    char buf[BUFSIZE];
    int32_t head;
    int32_t tail;
};

void ioq_init(struct ioqueue* ioq);
int ioq_full(struct ioqueue* ioq);
int ioq_empty(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);

#endif
