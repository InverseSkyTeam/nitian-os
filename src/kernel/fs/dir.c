// 参考: 《操作系统真相还原》(于渊) 第14章 文件系统
#include "dir.h"
#include "super_block.h"
#include "inode.h"
#include "fs.h"
#include "../device/ide.h"
#include "../memory/pool/pool.h"
#include "../lib/str/str.h"
#include "../include/assert.h"

struct dir root_dir;

void open_root_dir(struct partition* part) {
    root_dir.inode = inode_open(part, part->sb->root_inode_no);
    root_dir.dir_pos = 0;
}

struct dir* dir_open(struct partition* part, uint32_t inode_no) {
    struct dir* pdir = (struct dir*)get_kernel_pages(1);
    memset(pdir, 0, PAGE_SIZE);
    pdir->inode = inode_open(part, inode_no);
    pdir->dir_pos = 0;
    return pdir;
}

void dir_close(struct dir* dir) {
    if (dir == &root_dir) {
        return;
    }
    inode_close(dir->inode);
}

int dir_lookup(struct dir* pdir, const char* name, struct dir_entry* dir_e) {
    uint32_t block_cnt = 140;
    uint32_t* all_blocks = (uint32_t*)get_kernel_pages(1);
    memset(all_blocks, 0, PAGE_SIZE);
    uint32_t block_idx = 0;
    for (block_idx = 0; block_idx < 12; block_idx++) {
        all_blocks[block_idx] = pdir->inode->i_sectors[block_idx];
    }
    if (pdir->inode->i_sectors[12] != 0) {
        ide_read(cur_part->my_disk, pdir->inode->i_sectors[12], all_blocks + 12, 1);
    }
    uint8_t* buf = (uint8_t*)get_kernel_pages(1);
    memset(buf, 0, PAGE_SIZE);
    uint32_t dir_entry_size = cur_part->sb->dir_entry_size;
    uint32_t dir_entry_cnt = BLOCK_SIZE / dir_entry_size;
    for (block_idx = 0; block_idx < block_cnt; block_idx++) {
        if (all_blocks[block_idx] == 0) {
            continue;
        }
        ide_read(cur_part->my_disk, all_blocks[block_idx], buf, 1);
        struct dir_entry* p_de = (struct dir_entry*)buf;
        uint32_t i = 0;
        for (i = 0; i < dir_entry_cnt; i++) {
            if (strcmp(p_de->filename, name) == 0) {
                memcpy(dir_e, p_de, dir_entry_size);
                return 1;
            }
            p_de++;
        }
    }
    return -1;
}

struct dir_entry* dir_read(struct dir* dir) {
    struct dir_entry* dir_e = (struct dir_entry*)dir->dir_buf;
    struct inode* dir_inode = dir->inode;
    uint32_t all_blocks[140] = {0};
    uint32_t block_idx = 0;
    uint32_t block_cnt = 12;
    for (block_idx = 0; block_idx < 12; block_idx++) {
        all_blocks[block_idx] = dir_inode->i_sectors[block_idx];
    }
    if (dir_inode->i_sectors[12] != 0) {
        ide_read(cur_part->my_disk, dir_inode->i_sectors[12], all_blocks + 12, 1);
        block_cnt = 140;
    }
    block_idx = 0;
    uint32_t dir_entry_size = cur_part->sb->dir_entry_size;
    uint32_t dir_entry_cnt = BLOCK_SIZE / dir_entry_size;
    while (dir->dir_pos < dir_inode->i_size) {
        if (all_blocks[block_idx] == 0) {
            block_idx++;
            continue;
        }
        memset(dir_e, 0, BLOCK_SIZE);
        ide_read(cur_part->my_disk, all_blocks[block_idx], dir_e, 1);
        uint32_t i = 0;
        for (i = 0; i < dir_entry_cnt; i++) {
            if ((dir_e + i)->f_type) {
                if ((uint32_t)(i * dir_entry_size) < dir->dir_pos) {
                    continue;
                }
                dir->dir_pos += dir_entry_size;
                return dir_e + i;
            }
        }
        block_idx++;
    }
    return NULL;
}

int dir_is_empty(struct dir* dir) {
    return (dir->inode->i_size == cur_part->sb->dir_entry_size * 2);
}

int32_t dir_remove(struct dir* parent_dir, struct dir* child_dir) {
    struct inode* child_dir_inode = child_dir->inode;
    int32_t block_idx = 1;
    while (block_idx < 13) {
        ASSERT(child_dir_inode->i_sectors[block_idx] == 0);
        block_idx++;
    }
    uint8_t* io_buf = (uint8_t*)get_kernel_pages(1);
    memset(io_buf, 0, PAGE_SIZE);
    delete_dir_entry(cur_part, parent_dir, child_dir_inode->i_no, io_buf);
    inode_sync(cur_part, parent_dir->inode, io_buf);
    inode_release(cur_part, child_dir_inode);
    return 0;
}
