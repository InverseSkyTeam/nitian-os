// 参考: 《操作系统真相还原》(于渊) 第12章 系统调用

#include "./syscall.h"

static inline uint32_t syscall0(uint32_t nr) {
    uint32_t retval;
    __asm__ volatile("int $0x80"
                     : "=a"(retval)
                     : "a"(nr)
                     : "memory");
    return retval;
}

static inline uint32_t syscall1(uint32_t nr, uint32_t arg1) {
    uint32_t retval;
    __asm__ volatile("int $0x80"
                     : "=a"(retval)
                     : "a"(nr), "b"(arg1)
                     : "memory");
    return retval;
}

uint32_t getpid(void) {
    return syscall0(SYS_GETPID);
}

uint32_t write(char* str) {
    return syscall1(SYS_WRITE, (uint32_t)str);
}
