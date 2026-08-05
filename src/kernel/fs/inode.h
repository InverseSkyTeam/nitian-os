// 参考: 《操作系统真相还原》(于渊) 第14章 文件系统
#ifndef FS_INODE_H
#define FS_INODE_H

#include <stdint.h>
#include "../lib/list/list.h"

struct inode {
    uint32_t i_no;
    uint32_t i_size;
    uint32_t i_open_cnt;
    uint8_t write_deny;
    uint32_t i_sectors[13];
    struct list_elem inode_tag;
};

struct inode_position {
    int two_sec;
    uint32_t sec_lba;
    uint32_t off_size;
};

struct partition;

struct inode* inode_open(struct partition* part, uint32_t inode_no);
void inode_close(struct inode* inode);
void inode_sync(struct partition* part, struct inode* inode, void* io_buf);
void inode_release(struct partition* part, struct inode* inode);

#endif
