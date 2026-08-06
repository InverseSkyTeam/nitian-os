// 参考: 《操作系统真相还原》(于渊) 第12章 系统调用

#ifndef SYSCALL_NR_H
#define SYSCALL_NR_H

enum syscall_nr {
    SYS_GETPID,
    SYS_WRITE,
    SYS_READ,
    SYS_PUTCHAR,
    SYS_CLEAR,
    SYS_FORK,
    SYS_GETCWD,
    SYS_CHDIR,
    SYS_MKDIR,
    SYS_RMDIR,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_LSEEK,
    SYS_UNLINK,
    SYS_OPENDIR,
    SYS_CLOSEDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    SYS_STAT,
    SYS_PS,
    SYS_EXECV,
    SYS_EXIT,
    SYS_WAIT,
    SYS_PIPE,
    SYS_FD_REDIRECT
};

#endif
