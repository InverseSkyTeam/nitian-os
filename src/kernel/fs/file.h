// 参考: 《操作系统真相还原》(于渊) 第14章 文件系统
#ifndef FS_FILE_H
#define FS_FILE_H

#include <stdint.h>
#include "inode.h"
#include "fs.h"

#define MAX_FILE_OPEN 32

struct file {
    uint32_t fd_pos;
    uint32_t fd_flag;
    struct inode* fd_inode;
};

extern struct file file_table[MAX_FILE_OPEN];

int fd_install(int32_t global_fd_idx);
int fd_release(uint32_t local_fd);
uint32_t fd_local2global(uint32_t local_fd);
uint32_t file_read(struct file* file, void* buf, uint32_t count);
uint32_t file_write(struct file* file, const void* buf, uint32_t count);

#endif
