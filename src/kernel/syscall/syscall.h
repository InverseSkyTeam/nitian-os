// 参考: 《操作系统真相还原》(于渊) 第12章 系统调用
#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "../include/asm/stub.h"
#include "../include/syscall_nr.h"

#define SYSCALL_NR_MAX 32

void syscall_init(void);

uint32_t syscall_handler(struct Registers* r);

#endif
