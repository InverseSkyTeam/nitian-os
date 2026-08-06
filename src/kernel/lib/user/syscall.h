// 参考: 《操作系统真相还原》(于渊) 第12章 系统调用

#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include <stdint.h>
#include "../../include/syscall_nr.h"

struct stat;
struct dir;
struct dir_entry;

uint32_t getpid(void);
uint32_t write(char* str);
int32_t  read(int32_t fd, void* buf, uint32_t count);
void     putchar(char c);
void     clear(void);
int32_t  fork(void);
int32_t  open(const char* pathname, uint8_t flag);
int32_t  close(int32_t fd);
int32_t  lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t  unlink(const char* pathname);
int32_t  mkdir(const char* pathname);
int32_t  rmdir(const char* pathname);
int32_t  chdir(const char* path);
char*    getcwd(char* buf, uint32_t size);
int32_t  stat(const char* path, struct stat* buf);
struct dir* opendir(const char* name);
int32_t  closedir(struct dir* dir);
struct dir_entry* readdir(struct dir* dir);
void     rewinddir(struct dir* dir);
void     ps(void);

#endif
