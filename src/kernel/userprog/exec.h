// 参考: 《操作系统真相还原》(于渊) 第15章 加载用户进程
#ifndef EXEC_H
#define EXEC_H

#include <stdint.h>

int32_t sys_execv(const char* path, const char* argv[]);

#endif
