// 参考: 《操作系统真相还原》(于渊) 第11章 用户进程
#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "../thread/thread.h"

#define USER_VADDR_START 0x8048000
#define USER_STACK3_VADDR (0xc0000000 - 0x1000)
#define DEFAULT_PRIO 15

extern void intr_exit(void);

void start_process(void* filename_);
void page_dir_activate(struct task_struct* pthread);
void process_activate(struct task_struct* pthread);
uint32_t* create_page_dir(void);
void create_user_vaddr_bitmap(struct task_struct* user_prog);
void process_execute(void* filename, char* name);

#endif
