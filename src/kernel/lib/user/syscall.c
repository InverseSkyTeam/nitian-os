// 参考: 《操作系统真相还原》(于渊) 第12章 系统调用

#include "./syscall.h"
#include "../../include/syscall_nr.h"

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

static inline uint32_t syscall2(uint32_t nr, uint32_t arg1, uint32_t arg2) {
    uint32_t retval;
    __asm__ volatile("int $0x80"
                     : "=a"(retval)
                     : "a"(nr), "b"(arg1), "c"(arg2)
                     : "memory");
    return retval;
}

static inline uint32_t syscall3(uint32_t nr, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    uint32_t retval;
    __asm__ volatile("int $0x80"
                     : "=a"(retval)
                     : "a"(nr), "b"(arg1), "c"(arg2), "d"(arg3)
                     : "memory");
    return retval;
}

uint32_t getpid(void)                 { return syscall0(SYS_GETPID); }
uint32_t write(char* str)             { return syscall1(SYS_WRITE, (uint32_t)str); }
int32_t  read(int32_t fd, void* buf, uint32_t count) {
    return (int32_t)syscall3(SYS_READ, (uint32_t)fd, (uint32_t)buf, count);
}
void     putchar(char c)              { syscall1(SYS_PUTCHAR, (uint32_t)(unsigned char)c); }
void     clear(void)                  { syscall0(SYS_CLEAR); }
int32_t  fork(void)                   { return (int32_t)syscall0(SYS_FORK); }
int32_t  open(const char* pathname, uint8_t flag) {
    return (int32_t)syscall2(SYS_OPEN, (uint32_t)pathname, (uint32_t)flag);
}
int32_t  close(int32_t fd)            { return (int32_t)syscall1(SYS_CLOSE, (uint32_t)fd); }
int32_t  lseek(int32_t fd, int32_t offset, uint8_t whence) {
    return (int32_t)syscall3(SYS_LSEEK, (uint32_t)fd, (uint32_t)offset, (uint32_t)whence);
}
int32_t  unlink(const char* pathname) { return (int32_t)syscall1(SYS_UNLINK, (uint32_t)pathname); }
int32_t  mkdir(const char* pathname)  { return (int32_t)syscall1(SYS_MKDIR, (uint32_t)pathname); }
int32_t  rmdir(const char* pathname)  { return (int32_t)syscall1(SYS_RMDIR, (uint32_t)pathname); }
int32_t  chdir(const char* path)      { return (int32_t)syscall1(SYS_CHDIR, (uint32_t)path); }
char*    getcwd(char* buf, uint32_t size) { return (char*)syscall2(SYS_GETCWD, (uint32_t)buf, size); }
int32_t  stat(const char* path, struct stat* buf) {
    return (int32_t)syscall2(SYS_STAT, (uint32_t)path, (uint32_t)buf);
}
struct dir* opendir(const char* name) { return (struct dir*)syscall1(SYS_OPENDIR, (uint32_t)name); }
int32_t  closedir(struct dir* dir)    { return (int32_t)syscall1(SYS_CLOSEDIR, (uint32_t)dir); }
struct dir_entry* readdir(struct dir* dir) {
    return (struct dir_entry*)syscall1(SYS_READDIR, (uint32_t)dir);
}
void     rewinddir(struct dir* dir)   { syscall1(SYS_REWINDDIR, (uint32_t)dir); }
void     ps(void)                     { syscall0(SYS_PS); }
