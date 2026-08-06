// 参考: 《操作系统真相还原》(于渊) 第15章 wait与exit系统调用
#ifndef WAIT_EXIT_H
#define WAIT_EXIT_H

#include <stdint.h>
#include "../thread/thread.h"

pid_t sys_wait(int32_t* status);
void sys_exit(int32_t status);

#endif
