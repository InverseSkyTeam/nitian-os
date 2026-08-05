// 参考: 《操作系统真相还原》(于渊) 第14章 文件系统
#ifndef FS_DIR_H
#define FS_DIR_H

#include <stdint.h>
#include "inode.h"
#include "fs.h"

#define MAX_FILE_NAME_LEN 16

struct dir {
    struct inode* inode;
    uint32_t dir_pos;
    uint8_t dir_buf[512];
};

struct dir_entry {
    char filename[MAX_FILE_NAME_LEN];
    uint32_t i_no;
    enum file_types f_type;
};

extern struct dir root_dir;

void open_root_dir(struct partition* part);
struct dir* dir_open(struct partition* part, uint32_t inode_no);
void dir_close(struct dir* dir);
int dir_lookup(struct dir* pdir, const char* name, struct dir_entry* dir_e);
struct dir_entry* dir_read(struct dir* dir);
int dir_is_empty(struct dir* dir);
int32_t dir_remove(struct dir* parent_dir, struct dir* child_dir);

#endif
