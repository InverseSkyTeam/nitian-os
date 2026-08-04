// 参考: 《操作系统真相还原》(于渊) 第12章 系统调用

#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include <stdint.h>
#include "../../include/syscall_nr.h"

uint32_t getpid(void);        
uint32_t write(char* str);    

#endif
