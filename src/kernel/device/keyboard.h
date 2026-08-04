// 参考: 《操作系统真相还原》(于渊) 第11章 输入输出系统
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "./ioqueue.h"

extern struct ioqueue keyboard_ioq;

void keyboard_init(void);
void keyboard_handler(void);

#endif
