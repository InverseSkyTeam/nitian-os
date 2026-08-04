// 参考: 《操作系统真相还原》(于渊) 第12章 系统调用
#include "./syscall.h"
#include "../thread/thread.h"
#include "../initer/io/io.h"
#include "../lib/str/str.h"

static uint32_t sys_getpid(void) {
    return current_task->pid;
}

static uint32_t sys_write(char* str) {
    uint32_t len = 0;
    while (str[len] != 0) {
        console_putc(str[len]);
        len++;
    }
    return len;
}

uint32_t syscall_handler(struct Registers* r) {
    uint32_t nr = r->eax;
    switch (nr) {
    case SYS_GETPID:
        return sys_getpid();
    case SYS_WRITE:
        return sys_write((char*)r->ebx);
    default:
        return (uint32_t)-1;
    }
}

void syscall_init(void) {
    kprintf("[OK] syscall init, 0x80 registered\n");
}
