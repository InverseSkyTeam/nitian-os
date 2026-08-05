// 参考: 《操作系统真相还原》(于渊) 第14章 文件系统
#include "file.h"
#include "fs.h"
#include "inode.h"
#include "../device/ide.h"
#include "../thread/thread.h"
#include "../memory/pool/pool.h"
#include "../lib/str/str.h"
#include "../include/assert.h"

struct file file_table[MAX_FILE_OPEN];

int fd_install(int32_t global_fd_idx) {
    uint32_t local_fd = 3;
    while (local_fd < MAX_FILES_OPEN_PER_PROC) {
        if (current_task->fd_table[local_fd] == (uint32_t)-1) {
            current_task->fd_table[local_fd] = (uint32_t)global_fd_idx;
            return local_fd;
        }
        local_fd++;
    }
    return -1;
}

int fd_release(uint32_t local_fd) {
    if (local_fd >= MAX_FILES_OPEN_PER_PROC) {
        return -1;
    }
    current_task->fd_table[local_fd] = (uint32_t)-1;
    return 0;
}

uint32_t file_read(struct file* file, void* buf, uint32_t count) {
    uint32_t size = file->fd_inode->i_size;
    if (file->fd_pos >= size) {
        return 0;
    }
    uint32_t read_size = size - file->fd_pos;
    if (count < read_size) {
        read_size = count;
    }
    uint8_t* block_buf = (uint8_t*)get_kernel_pages(1);
    memset(block_buf, 0, PAGE_SIZE);
    uint32_t* all_blocks = (uint32_t*)get_kernel_pages(1);
    memset(all_blocks, 0, PAGE_SIZE);
    uint32_t block_idx = 0;
    for (block_idx = 0; block_idx < 12; block_idx++) {
        all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
    }
    if (file->fd_inode->i_sectors[12] != 0) {
        ide_read(cur_part->my_disk, file->fd_inode->i_sectors[12], all_blocks + 12, 1);
    }
    uint32_t bytes_read = 0;
    uint32_t start_block = file->fd_pos / BLOCK_SIZE;
    uint32_t start_off = file->fd_pos % BLOCK_SIZE;
    block_idx = start_block;
    uint32_t off = start_off;
    while (bytes_read < read_size) {
        if (all_blocks[block_idx] == 0) {
            break;
        }
        ide_read(cur_part->my_disk, all_blocks[block_idx], block_buf, 1);
        uint32_t chunk = BLOCK_SIZE - off;
        if (chunk > read_size - bytes_read) {
            chunk = read_size - bytes_read;
        }
        memcpy((uint8_t*)buf + bytes_read, block_buf + off, chunk);
        bytes_read += chunk;
        block_idx++;
        off = 0;
    }
    file->fd_pos += bytes_read;
    return bytes_read;
}

uint32_t file_write(struct file* file, const void* buf, uint32_t count) {
    if ((file->fd_pos + count) > BLOCK_SIZE * 140) {
        return (uint32_t)-1;
    }
    uint32_t bytes_written = 0;
    uint8_t* block_buf = (uint8_t*)get_kernel_pages(1);
    memset(block_buf, 0, PAGE_SIZE);
    uint32_t* all_blocks = (uint32_t*)get_kernel_pages(1);
    memset(all_blocks, 0, PAGE_SIZE);
    uint32_t block_idx = 0;
    for (block_idx = 0; block_idx < 12; block_idx++) {
        all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
    }
    if (file->fd_inode->i_sectors[12] != 0) {
        ide_read(cur_part->my_disk, file->fd_inode->i_sectors[12], all_blocks + 12, 1);
    }
    while (count > 0) {
        block_idx = file->fd_pos / BLOCK_SIZE;
        uint32_t off = file->fd_pos % BLOCK_SIZE;
        if (block_idx >= 140) {
            return (uint32_t)-1;
        }
        if (all_blocks[block_idx] == 0) {
            if (block_idx < 12) {
                uint32_t block_lba = (uint32_t)block_bitmap_alloc(cur_part);
                file->fd_inode->i_sectors[block_idx] = block_lba;
                all_blocks[block_idx] = block_lba;
            } else {
                if (file->fd_inode->i_sectors[12] == 0) {
                    uint32_t indirect_lba = (uint32_t)block_bitmap_alloc(cur_part);
                    file->fd_inode->i_sectors[12] = indirect_lba;
                    all_blocks[12] = indirect_lba;
                    memset(block_buf, 0, BLOCK_SIZE);
                    ide_write(cur_part->my_disk, indirect_lba, block_buf, 1);
                }
                ide_read(cur_part->my_disk, file->fd_inode->i_sectors[12], block_buf, 1);
                uint32_t block_lba = (uint32_t)block_bitmap_alloc(cur_part);
                ((uint32_t*)block_buf)[block_idx - 12] = block_lba;
                ide_write(cur_part->my_disk, file->fd_inode->i_sectors[12], block_buf, 1);
                all_blocks[block_idx] = block_lba;
            }
            uint8_t* io_buf = (uint8_t*)get_kernel_pages(1);
            inode_sync(cur_part, file->fd_inode, io_buf);
            memset(block_buf, 0, BLOCK_SIZE);
        }
        uint32_t block_lba = all_blocks[block_idx];
        memset(block_buf, 0, BLOCK_SIZE);
        ide_read(cur_part->my_disk, block_lba, block_buf, 1);
        uint32_t chunk = BLOCK_SIZE - off;
        if (chunk > count) {
            chunk = count;
        }
        memcpy(block_buf + off, (const uint8_t*)buf + bytes_written, chunk);
        ide_write(cur_part->my_disk, block_lba, block_buf, 1);
        file->fd_pos += chunk;
        bytes_written += chunk;
        count -= chunk;
        if (file->fd_pos > file->fd_inode->i_size) {
            file->fd_inode->i_size = file->fd_pos;
        }
    }
    uint8_t* io_buf = (uint8_t*)get_kernel_pages(1);
    inode_sync(cur_part, file->fd_inode, io_buf);
    return bytes_written;
}
