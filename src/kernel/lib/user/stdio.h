// 参考: 《操作系统真相还原》(于渊) 第12章 格式化输出

#ifndef USER_STDIO_H
#define USER_STDIO_H

#include <stdint.h>
#include <stdarg.h>

uint32_t vsprintf(char* str, const char* format, va_list ap);

void printf(const char* format, ...);

uint32_t sprintf(char* buf, const char* format, ...);

#endif
