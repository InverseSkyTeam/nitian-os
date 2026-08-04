// 参考: 《操作系统真相还原》(于渊) 第12章 系统调用

#ifndef SYSCALL_NR_H
#define SYSCALL_NR_H

enum syscall_nr {
    SYS_GETPID,
    SYS_WRITE
};

#endif
