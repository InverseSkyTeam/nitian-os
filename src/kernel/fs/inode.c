// 参考: 《操作系统真相还原》(于渊) 第14章 文件系统
#include "inode.h"
#include "super_block.h"
#include "fs.h"
#include "../device/ide.h"
#include "../memory/pool/pool.h"
#include "../lib/str/str.h"
#include "../include/asmFunc.h"
#include "../include/assert.h"

static void inode_locate(struct partition* part, uint32_t inode_no, struct inode_position* inode_pos) {
    ASSERT(inode_no < MAX_FILES_PER_PART);
    uint32_t inode_size = sizeof(struct inode);
    uint32_t byte_cnt = inode_no * inode_size;
    uint32_t off_sec = byte_cnt / SECTOR_SIZE;
    uint32_t off_size = byte_cnt % SECTOR_SIZE;
    inode_pos->sec_lba = part->sb->inode_table_lba + off_sec;
    inode_pos->off_size = off_size;
    inode_pos->two_sec = (off_size + inode_size > SECTOR_SIZE);
}

struct inode* inode_open(struct partition* part, uint32_t inode_no) {
    struct list_elem* elem = part->open_inodes.head.next;
    while (elem != &part->open_inodes.tail) {
        struct inode* inode = list_entry(elem, struct inode, inode_tag);
        if (inode->i_no == inode_no) {
            inode->i_open_cnt++;
            return inode;
        }
        elem = elem->next;
    }
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    uint8_t* buf = (uint8_t*)get_kernel_pages(1);
    memset(buf, 0, PAGE_SIZE);
    if (inode_pos.two_sec) {
        ide_read(part->my_disk, inode_pos.sec_lba, buf, 2);
    } else {
        ide_read(part->my_disk, inode_pos.sec_lba, buf, 1);
    }
    struct inode* inode = (struct inode*)get_kernel_pages(1);
    memset(inode, 0, PAGE_SIZE);
    memcpy(inode, buf + inode_pos.off_size, sizeof(struct inode));
    inode->i_no = inode_no;
    inode->i_open_cnt = 1;
    inode->write_deny = 0;
    list_append(&part->open_inodes, &inode->inode_tag);
    return inode;
}

void inode_close(struct inode* inode) {
    uint32_t old = asm_save_eflags();
    asm_cli();
    if (--inode->i_open_cnt == 0) {
        list_remove(&inode->inode_tag);
        inode->i_open_cnt = 0;
    }
    asm_restore_eflags(old);
}

void inode_sync(struct partition* part, struct inode* inode, void* io_buf) {
    uint32_t inode_no = inode->i_no;
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    struct inode pure_inode;
    memcpy(&pure_inode, inode, sizeof(struct inode));
    pure_inode.i_open_cnt = 0;
    pure_inode.write_deny = 0;
    pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;
    if (inode_pos.two_sec) {
        ide_read(part->my_disk, inode_pos.sec_lba, io_buf, 2);
        memcpy((uint8_t*)io_buf + inode_pos.off_size, &pure_inode, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, io_buf, 2);
    } else {
        ide_read(part->my_disk, inode_pos.sec_lba, io_buf, 1);
        memcpy((uint8_t*)io_buf + inode_pos.off_size, &pure_inode, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, io_buf, 1);
    }
}

void inode_release(struct partition* part, struct inode* inode) {
    uint32_t block_idx = 0;
    uint32_t block_cnt = 12;
    uint32_t* all_blocks = (uint32_t*)get_kernel_pages(1);
    memset(all_blocks, 0, PAGE_SIZE);
    for (block_idx = 0; block_idx < 12; block_idx++) {
        all_blocks[block_idx] = inode->i_sectors[block_idx];
    }
    if (inode->i_sectors[12] != 0) {
        ide_read(part->my_disk, inode->i_sectors[12], all_blocks + 12, 1);
    }
    for (block_idx = 0; block_idx < block_cnt; block_idx++) {
        if (all_blocks[block_idx] != 0) {
            block_bitmap_free(part, all_blocks[block_idx]);
        }
    }
    if (inode->i_sectors[12] != 0) {
        block_bitmap_free(part, inode->i_sectors[12]);
    }
    inode_bitmap_free(part, inode->i_no);
}
