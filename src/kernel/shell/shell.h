// 参考: 《操作系统真相还原》(于渊) 第15章 系统交互
#ifndef SHELL_H
#define SHELL_H

#include "../fs/fs.h"

void print_prompt(void);
void my_shell(void* arg);

extern char final_path[MAX_PATH_LEN];

#endif
