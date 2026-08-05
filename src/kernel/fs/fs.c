// 参考: 《操作系统真相还原》(于渊) 第14章 文件系统
#include "fs.h"
#include "super_block.h"
#include "inode.h"
#include "dir.h"
#include "file.h"
#include "../device/ide.h"
#include "../thread/thread.h"
#include "../memory/pool/pool.h"
#include "../lib/str/str.h"
#include "../initer/io/io.h"
#include "../include/assert.h"

#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / STEP)

struct partition* cur_part;

static void partition_format(struct partition* part) {
    uint32_t boot_sector_sects = 1;
    uint32_t super_block_sects = 1;
    uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);
    uint32_t inode_table_sects = DIV_ROUND_UP((sizeof(struct inode) * MAX_FILES_PER_PART), SECTOR_SIZE);
    uint32_t used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
    uint32_t free_sects = part->sec_cnt - used_sects;

    uint32_t block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
    uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
    block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR);

    struct super_block sb;
    sb.magic = FS_MAGIC;
    sb.sec_cnt = part->sec_cnt;
    sb.inode_cnt = MAX_FILES_PER_PART;
    sb.part_lba_base = part->start_lba;

    sb.block_bitmap_lba = sb.part_lba_base + 2;
    sb.block_bitmap_sects = block_bitmap_sects;

    sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
    sb.inode_bitmap_sects = inode_bitmap_sects;

    sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
    sb.inode_table_sects = inode_table_sects;

    sb.data_start_lba = sb.inode_table_lba + sb.inode_table_sects;
    sb.root_inode_no = 0;
    sb.dir_entry_size = sizeof(struct dir_entry);

    kprintf("%s info:\n", part->name);
    kprintf("   magic:0x%x part_lba_base:0x%x all_sectors:0x%x inode_cnt:0x%x\n",
            (unsigned)sb.magic, (unsigned)sb.part_lba_base, (unsigned)sb.sec_cnt, (unsigned)sb.inode_cnt);
    kprintf("   block_bitmap_lba:0x%x sects:0x%x\n",
            (unsigned)sb.block_bitmap_lba, (unsigned)sb.block_bitmap_sects);
    kprintf("   inode_bitmap_lba:0x%x sects:0x%x\n",
            (unsigned)sb.inode_bitmap_lba, (unsigned)sb.inode_bitmap_sects);
    kprintf("   inode_table_lba:0x%x sects:0x%x\n",
            (unsigned)sb.inode_table_lba, (unsigned)sb.inode_table_sects);
    kprintf("   data_start_lba:0x%x\n", (unsigned)sb.data_start_lba);

    struct disk* hd = part->my_disk;
    ide_write(hd, part->start_lba + 1, &sb, 1);
    kprintf("   super_block_lba:0x%x\n", (unsigned)(part->start_lba + 1));
    uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects ? sb.block_bitmap_sects : sb.inode_bitmap_sects);
    buf_size = (buf_size >= sb.inode_table_sects ? buf_size : sb.inode_table_sects) * SECTOR_SIZE;
    uint8_t* buf = (uint8_t*)get_kernel_pages(DIV_ROUND_UP(buf_size, PAGE_SIZE));
    memset(buf, 0, buf_size);

    buf[0] |= 0x80;
    uint32_t block_bitmap_last_byte = block_bitmap_bit_len / 8;
    uint8_t block_bitmap_last_bit = (uint8_t)(block_bitmap_bit_len % 8);
    uint32_t last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE);
    memset(&buf[block_bitmap_last_byte], 0xff, last_size);
    uint8_t bit_idx = 0;
    while (bit_idx <= block_bitmap_last_bit) {
        buf[block_bitmap_last_byte] &= (uint8_t)~(0x80 >> bit_idx++);
    }
    ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

    memset(buf, 0, buf_size);
    buf[0] |= 0x80;
    ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);

    memset(buf, 0, buf_size);
    struct inode* i = (struct inode*)buf;
    i->i_size = sb.dir_entry_size * 2;
    i->i_no = 0;
    i->i_sectors[0] = sb.data_start_lba;
    ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);

    memset(buf, 0, buf_size);
    struct dir_entry* p_de = (struct dir_entry*)buf;
    memcpy(p_de->filename, ".", 1);
    p_de->i_no = 0;
    p_de->f_type = FT_DIRECTORY;
    ++p_de;
    memcpy(p_de->filename, "..", 2);
    p_de->i_no = 0;
    p_de->f_type = FT_DIRECTORY;
    ide_write(hd, sb.data_start_lba, buf, 1);

    kprintf("   root_dir_lba:0x%x\n", (unsigned)sb.data_start_lba);
    kprintf("%s format done\n", part->name);
}

static void mount_partition(struct partition* part) {
    struct disk* hd = part->my_disk;
    struct super_block* sb_buf = (struct super_block*)get_kernel_pages(1);
    memset(sb_buf, 0, SECTOR_SIZE);
    ide_read(hd, part->start_lba + 1, sb_buf, 1);

    part->sb = (struct super_block*)get_kernel_pages(1);
    memcpy(part->sb, sb_buf, sizeof(struct super_block));

    part->block_bitmap.bits = (uint8_t*)get_kernel_pages(DIV_ROUND_UP(sb_buf->block_bitmap_sects * SECTOR_SIZE, PAGE_SIZE));
    part->block_bitmap.btmp_bytes_len = sb_buf->block_bitmap_sects * SECTOR_SIZE;
    ide_read(hd, sb_buf->block_bitmap_lba, part->block_bitmap.bits, sb_buf->block_bitmap_sects);

    part->inode_bitmap.bits = (uint8_t*)get_kernel_pages(DIV_ROUND_UP(sb_buf->inode_bitmap_sects * SECTOR_SIZE, PAGE_SIZE));
    part->inode_bitmap.btmp_bytes_len = sb_buf->inode_bitmap_sects * SECTOR_SIZE;
    ide_read(hd, sb_buf->inode_bitmap_lba, part->inode_bitmap.bits, sb_buf->inode_bitmap_sects);

    list_init(&part->open_inodes);
    open_root_dir(part);
    kprintf("mount %s done!\n", part->name);
}

void filesys_init(void) {
    kprintf("searching filesystem......\n");
    struct super_block* sb_buf = (struct super_block*)get_kernel_pages(1);

    struct list_elem* e = partition_list.head.next;
    while (e != &partition_list.tail) {
        struct partition* part = list_entry(e, struct partition, part_tag);
        memset(sb_buf, 0, SECTOR_SIZE);
        ide_read(part->my_disk, part->start_lba + 1, sb_buf, 1);
        if (sb_buf->magic == FS_MAGIC) {
            kprintf("%s has filesystem\n", part->name);
        } else {
            kprintf("formatting %s partition %s.....\n", part->my_disk->name, part->name);
            partition_format(part);
        }
        e = e->next;
    }

    struct list_elem* pe = partition_list.head.next;
    while (pe != &partition_list.tail) {
        struct partition* part = list_entry(pe, struct partition, part_tag);
        if (strcmp(part->name, "sda1") == 0) {
            cur_part = part;
            mount_partition(part);
            break;
        }
        pe = pe->next;
    }
}

int32_t block_bitmap_alloc(struct partition* part) {
    int32_t bit_idx = bitmap_scan(&part->block_bitmap, 1);
    if (bit_idx == -1) {
        return -1;
    }
    bitmap_set(&part->block_bitmap, (uint32_t)bit_idx, 1);
    return (int32_t)(part->sb->data_start_lba + (uint32_t)bit_idx);
}

void block_bitmap_free(struct partition* part, uint32_t lba) {
    uint32_t bit_idx = lba - part->sb->data_start_lba;
    bitmap_set(&part->block_bitmap, bit_idx, 0);
}

int32_t inode_bitmap_alloc(struct partition* part) {
    int32_t bit_idx = bitmap_scan(&part->inode_bitmap, 1);
    if (bit_idx == -1) {
        return -1;
    }
    bitmap_set(&part->inode_bitmap, (uint32_t)bit_idx, 1);
    return bit_idx;
}

void inode_bitmap_free(struct partition* part, uint32_t inode_no) {
    bitmap_set(&part->inode_bitmap, inode_no, 0);
}

char* path_parse(char* pathname, char* name_store) {
    uint32_t cnt = 0;
    if (pathname[0] == '/') {
        while (*(++pathname) == '/');
    }
    while (*pathname != '/' && *pathname != 0 && cnt < MAX_FILE_NAME_LEN - 1) {
        *name_store++ = *pathname++;
        cnt++;
    }
    if (pathname[0] == 0) {
        return NULL;
    }
    return pathname;
}

int search_dir_entry(struct partition* part, struct dir* pdir, const char* name, struct dir_entry* dir_e) {
    uint32_t block_cnt = 140;
    uint32_t* all_blocks = (uint32_t*)get_kernel_pages(1);
    memset(all_blocks, 0, PAGE_SIZE);
    uint32_t block_idx = 0;
    for (block_idx = 0; block_idx < 12; block_idx++) {
        all_blocks[block_idx] = pdir->inode->i_sectors[block_idx];
    }
    if (pdir->inode->i_sectors[12] != 0) {
        ide_read(part->my_disk, pdir->inode->i_sectors[12], all_blocks + 12, 1);
    }
    uint8_t* buf = (uint8_t*)get_kernel_pages(1);
    memset(buf, 0, PAGE_SIZE);
    uint32_t dir_entry_size = part->sb->dir_entry_size;
    uint32_t dir_entry_cnt = BLOCK_SIZE / dir_entry_size;
    for (block_idx = 0; block_idx < block_cnt; block_idx++) {
        if (all_blocks[block_idx] == 0) {
            continue;
        }
        ide_read(part->my_disk, all_blocks[block_idx], buf, 1);
        struct dir_entry* p_de = (struct dir_entry*)buf;
        uint32_t i = 0;
        for (i = 0; i < dir_entry_cnt; i++) {
            if (strcmp(p_de->filename, name) == 0) {
                memcpy(dir_e, p_de, dir_entry_size);
                return (int)p_de->i_no;
            }
            p_de++;
        }
    }
    return -1;
}

int create_dir_entry(struct partition* part, struct dir* pdir, uint32_t inode_no, const char* filename, enum file_types f_type) {
    uint32_t block_idx = 0;
    struct dir_entry* dir_e = NULL;
    uint32_t dir_size = pdir->inode->i_size;
    uint32_t dir_entry_size = part->sb->dir_entry_size;
    uint32_t dir_entry_cnt = BLOCK_SIZE / dir_entry_size;
    uint8_t* buf = (uint8_t*)get_kernel_pages(1);
    memset(buf, 0, PAGE_SIZE);
    uint32_t block_lba = 0;
    while (block_idx * BLOCK_SIZE < dir_size) {
        block_lba = pdir->inode->i_sectors[block_idx];
        ide_read(part->my_disk, block_lba, buf, 1);
        dir_e = (struct dir_entry*)buf;
        uint32_t i = 0;
        for (i = 0; i < dir_entry_cnt; i++) {
            if (dir_e->f_type == FT_UNKNOWN) {
                memcpy(dir_e->filename, filename, strlen(filename));
                dir_e->i_no = inode_no;
                dir_e->f_type = f_type;
                ide_write(part->my_disk, block_lba, buf, 1);
                uint32_t new_size = (block_idx * dir_entry_cnt + i + 1) * dir_entry_size;
                if (new_size > pdir->inode->i_size) {
                    pdir->inode->i_size = new_size;
                }
                return 1;
            }
            dir_e++;
        }
        block_idx++;
    }
    return 0;
}

int sync_dir_entry(struct dir* pdir, struct dir_entry* p_de, struct dir_entry* new_de) {
    uint32_t block_idx = 0;
    uint32_t block_cnt = 140;
    uint32_t* all_blocks = (uint32_t*)get_kernel_pages(1);
    memset(all_blocks, 0, PAGE_SIZE);
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
        struct dir_entry* p = (struct dir_entry*)buf;
        uint32_t i = 0;
        for (i = 0; i < dir_entry_cnt; i++) {
            if (p->i_no == p_de->i_no) {
                memcpy(p, new_de, dir_entry_size);
                ide_write(cur_part->my_disk, all_blocks[block_idx], buf, 1);
                return 1;
            }
            p++;
        }
    }
    return 0;
}

int delete_dir_entry(struct partition* part, struct dir* pdir, uint32_t inode_no, void* io_buf) {
    uint32_t block_idx = 0;
    uint32_t block_cnt = 140;
    uint32_t* all_blocks = (uint32_t*)get_kernel_pages(1);
    memset(all_blocks, 0, PAGE_SIZE);
    for (block_idx = 0; block_idx < 12; block_idx++) {
        all_blocks[block_idx] = pdir->inode->i_sectors[block_idx];
    }
    if (pdir->inode->i_sectors[12] != 0) {
        ide_read(part->my_disk, pdir->inode->i_sectors[12], all_blocks + 12, 1);
    }
    uint8_t* buf = (uint8_t*)get_kernel_pages(1);
    memset(buf, 0, PAGE_SIZE);
    uint32_t dir_entry_size = part->sb->dir_entry_size;
    uint32_t dir_entry_cnt = BLOCK_SIZE / dir_entry_size;
    for (block_idx = 0; block_idx < block_cnt; block_idx++) {
        if (all_blocks[block_idx] == 0) {
            continue;
        }
        ide_read(part->my_disk, all_blocks[block_idx], buf, 1);
        struct dir_entry* p_de = (struct dir_entry*)buf;
        uint32_t i = 0;
        for (i = 0; i < dir_entry_cnt; i++) {
            if (p_de->i_no == inode_no) {
                memset(p_de, 0, dir_entry_size);
                ide_write(part->my_disk, all_blocks[block_idx], buf, 1);
                uint32_t item_off = (block_idx * dir_entry_cnt + i) * dir_entry_size;
                if (item_off + dir_entry_size >= pdir->inode->i_size) {
                    pdir->inode->i_size = item_off;
                }
                return 1;
            }
            p_de++;
        }
    }
    return 0;
}

int search_file(const char* pathname, struct path_search_record* searched_record) {
    if (!strcmp(pathname, "/") || !strcmp(pathname, "/.") || !strcmp(pathname, "/..")) {
        searched_record->parent_dir = &root_dir;
        searched_record->file_type = FT_DIRECTORY;
        searched_record->searched_path[0] = 0;
        return 0;
    }
    uint32_t path_len = strlen(pathname);
    ASSERT(pathname[0] == '/' && path_len > 1);
    char* sub_path = (char*)get_kernel_pages(1);
    memset(sub_path, 0, PAGE_SIZE);
    strcpy(sub_path, pathname);
    char* path_ptr = sub_path;
    char name_store[MAX_FILE_NAME_LEN];
    struct dir* parent_dir = &root_dir;
    struct dir_entry dir_e;
    searched_record->searched_path[0] = 0;
    for (;;) {
        memset(name_store, 0, sizeof(name_store));
        path_ptr = path_parse(path_ptr, name_store);
        if (name_store[0] == 0) {
            break;
        }
        if (searched_record->searched_path[0] == 0) {
            strcpy(searched_record->searched_path, "/");
        } else {
            strcat(searched_record->searched_path, "/");
        }
        strcat(searched_record->searched_path, name_store);
        int sub_inode_no = search_dir_entry(cur_part, parent_dir, name_store, &dir_e);
        if (sub_inode_no == -1) {
            searched_record->parent_dir = parent_dir;
            searched_record->file_type = FT_UNKNOWN;
            return -1;
        }
        if (path_ptr == NULL) {
            searched_record->parent_dir = parent_dir;
            searched_record->file_type = dir_e.f_type;
            return sub_inode_no;
        }
        if (dir_e.f_type != FT_DIRECTORY) {
            return -1;
        }
        parent_dir = dir_open(cur_part, (uint32_t)sub_inode_no);
    }
    searched_record->parent_dir = parent_dir;
    searched_record->file_type = FT_UNKNOWN;
    return -1;
}

static void init_inode(struct inode* new_inode, uint32_t inode_no) {
    new_inode->i_no = inode_no;
    new_inode->i_size = 0;
    new_inode->i_open_cnt = 0;
    new_inode->write_deny = 0;
    uint32_t sec_idx = 0;
    while (sec_idx < 13) {
        new_inode->i_sectors[sec_idx] = 0;
        sec_idx++;
    }
}

int create_file(const char* pathname) {
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = search_file(pathname, &searched_record);
    if (inode_no != -1) {
        return -1;
    }
    int32_t fd_idx = inode_bitmap_alloc(cur_part);
    if (fd_idx == -1) {
        return -1;
    }
    struct inode* new_inode = (struct inode*)get_kernel_pages(1);
    memset(new_inode, 0, PAGE_SIZE);
    init_inode(new_inode, (uint32_t)fd_idx);
    struct dir* parent_dir = searched_record.parent_dir;
    uint8_t* buf = (uint8_t*)get_kernel_pages(1);
    memset(buf, 0, PAGE_SIZE);
    inode_sync(cur_part, new_inode, buf);
    char* filename = strrchr(searched_record.searched_path, '/');
    if (filename == NULL) {
        return -1;
    }
    filename++;
    if (!create_dir_entry(cur_part, parent_dir, (uint32_t)fd_idx, filename, FT_REGULAR)) {
        inode_bitmap_free(cur_part, (uint32_t)fd_idx);
        return -1;
    }
    inode_sync(cur_part, parent_dir->inode, buf);
    return 0;
}

int open_file(const char* pathname, uint8_t flags) {
    if (pathname[strlen(pathname) - 1] == '/') {
        return -1;
    }
    ASSERT(flags <= 7);
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = search_file(pathname, &searched_record);
    uint8_t fd_idx;
    if (inode_no == -1) {
        if (flags & O_CREAT) {
            if (!create_file(pathname)) {
                inode_no = search_file(pathname, &searched_record);
                if (inode_no == -1) {
                    return -1;
                }
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    }
    if (searched_record.file_type != FT_REGULAR) {
        return -1;
    }
    uint32_t global_fd_idx = 0;
    while (global_fd_idx < MAX_FILE_OPEN) {
        if (file_table[global_fd_idx].fd_inode == NULL) {
            break;
        }
        global_fd_idx++;
    }
    if (global_fd_idx >= MAX_FILE_OPEN) {
        return -1;
    }
    file_table[global_fd_idx].fd_pos = 0;
    file_table[global_fd_idx].fd_flag = flags;
    file_table[global_fd_idx].fd_inode = inode_open(cur_part, (uint32_t)inode_no);
    fd_idx = (uint8_t)fd_install((int32_t)global_fd_idx);
    if (fd_idx == (uint8_t)-1) {
        file_table[global_fd_idx].fd_inode = NULL;
        return -1;
    }
    return fd_idx;
}

int close_file(int fd) {
    if (fd < 3 || fd >= MAX_FILES_OPEN_PER_PROC) {
        return -1;
    }
    uint32_t global_fd_idx = current_task->fd_table[fd];
    if (global_fd_idx == (uint32_t)-1) {
        return -1;
    }
    struct file* file = &file_table[global_fd_idx];
    inode_close(file->fd_inode);
    file->fd_inode = NULL;
    file->fd_pos = 0;
    file->fd_flag = 0;
    fd_release((uint32_t)fd);
    return 0;
}

uint32_t read_file(int fd, void* buf, uint32_t count) {
    if (fd < 0 || fd >= MAX_FILES_OPEN_PER_PROC) {
        return (uint32_t)-1;
    }
    uint32_t global_fd_idx = current_task->fd_table[fd];
    if (global_fd_idx == (uint32_t)-1) {
        return (uint32_t)-1;
    }
    return file_read(&file_table[global_fd_idx], buf, count);
}

uint32_t write_file(int fd, const void* buf, uint32_t count) {
    if (fd < 0 || fd >= MAX_FILES_OPEN_PER_PROC) {
        return (uint32_t)-1;
    }
    uint32_t global_fd_idx = current_task->fd_table[fd];
    if (global_fd_idx == (uint32_t)-1) {
        return (uint32_t)-1;
    }
    return file_write(&file_table[global_fd_idx], buf, count);
}

int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence) {
    if (fd < 3 || fd >= MAX_FILES_OPEN_PER_PROC) {
        return -1;
    }
    uint32_t global_fd_idx = current_task->fd_table[fd];
    if (global_fd_idx == (uint32_t)-1) {
        return -1;
    }
    struct file* pf = &file_table[global_fd_idx];
    int32_t new_pos = 0;
    int32_t file_size = (int32_t)pf->fd_inode->i_size;
    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = (int32_t)pf->fd_pos + offset;
        break;
    case SEEK_END:
        new_pos = file_size + offset;
        break;
    default:
        return -1;
    }
    if (new_pos < 0 || new_pos > file_size) {
        return -1;
    }
    pf->fd_pos = (uint32_t)new_pos;
    return (int32_t)pf->fd_pos;
}

int sys_unlink(const char* pathname) {
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = search_file(pathname, &searched_record);
    if (inode_no == -1) {
        return -1;
    }
    if (searched_record.file_type != FT_REGULAR) {
        return -1;
    }
    struct inode* inode = inode_open(cur_part, (uint32_t)inode_no);
    if (inode->i_open_cnt > 1) {
        return -1;
    }
    uint8_t* io_buf = (uint8_t*)get_kernel_pages(1);
    memset(io_buf, 0, PAGE_SIZE);
    if (!delete_dir_entry(cur_part, searched_record.parent_dir, (uint32_t)inode_no, io_buf)) {
        return -1;
    }
    inode_sync(cur_part, searched_record.parent_dir->inode, io_buf);
    inode_release(cur_part, inode);
    inode_close(inode);
    return 0;
}

static uint32_t path_depth_cnt(char* pathname) {
    ASSERT(pathname != NULL);
    char* p = pathname;
    uint32_t cnt = 0;
    while (*p) {
        if (*p == '/') {
            cnt++;
        }
        p++;
    }
    return cnt;
}

int32_t sys_mkdir(const char* pathname) {
    uint8_t* io_buf = (uint8_t*)get_kernel_pages(1);
    memset(io_buf, 0, PAGE_SIZE);
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = search_file(pathname, &searched_record);
    if (inode_no != -1) {
        kprintf("sys_mkdir: %s exist!\n", pathname);
        return -1;
    }
    uint32_t pathname_depth = path_depth_cnt((char*)pathname);
    uint32_t path_searched_depth = path_depth_cnt(searched_record.searched_path);
    if (pathname_depth != path_searched_depth) {
        kprintf("sys_mkdir: subpath %s not exist\n", searched_record.searched_path);
        return -1;
    }
    struct dir* parent_dir = searched_record.parent_dir;
    char* dirname = strrchr(searched_record.searched_path, '/') + 1;

    inode_no = inode_bitmap_alloc(cur_part);
    if (inode_no == -1) {
        return -1;
    }
    struct inode new_dir_inode;
    init_inode(&new_dir_inode, (uint32_t)inode_no);

    int32_t block_lba = block_bitmap_alloc(cur_part);
    if (block_lba == -1) {
        inode_bitmap_free(cur_part, (uint32_t)inode_no);
        return -1;
    }
    new_dir_inode.i_sectors[0] = (uint32_t)block_lba;

    struct dir_entry* p_de = (struct dir_entry*)io_buf;
    memcpy(p_de->filename, ".", 1);
    p_de->i_no = (uint32_t)inode_no;
    p_de->f_type = FT_DIRECTORY;
    ++p_de;
    memcpy(p_de->filename, "..", 2);
    p_de->i_no = parent_dir->inode->i_no;
    p_de->f_type = FT_DIRECTORY;
    ide_write(cur_part->my_disk, new_dir_inode.i_sectors[0], io_buf, 1);

    new_dir_inode.i_size = 2 * cur_part->sb->dir_entry_size;

    if (!create_dir_entry(cur_part, parent_dir, (uint32_t)inode_no, dirname, FT_DIRECTORY)) {
        inode_bitmap_free(cur_part, (uint32_t)inode_no);
        block_bitmap_free(cur_part, (uint32_t)block_lba);
        return -1;
    }
    inode_sync(cur_part, parent_dir->inode, io_buf);
    inode_sync(cur_part, &new_dir_inode, io_buf);
    return 0;
}

struct dir* sys_opendir(const char* name) {
    if (name[0] == '/' && (name[1] == 0 || name[1] == '.')) {
        return &root_dir;
    }
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = search_file(name, &searched_record);
    struct dir* ret = NULL;
    if (inode_no == -1) {
        kprintf("sys_opendir: %s not exist\n", name);
    } else if (searched_record.file_type == FT_REGULAR) {
        kprintf("sys_opendir: %s is regular file!\n", name);
    } else {
        ret = dir_open(cur_part, (uint32_t)inode_no);
    }
    return ret;
}

int32_t sys_closedir(struct dir* dir) {
    int32_t ret = -1;
    if (dir != NULL) {
        dir_close(dir);
        ret = 0;
    }
    return ret;
}

struct dir_entry* sys_readdir(struct dir* dir) {
    return dir_read(dir);
}

void sys_rewinddir(struct dir* dir) {
    dir->dir_pos = 0;
}

int32_t sys_rmdir(const char* pathname) {
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = search_file(pathname, &searched_record);
    if (inode_no == -1) {
        kprintf("sys_rmdir: %s not exist\n", pathname);
        return -1;
    }
    ASSERT(inode_no != 0);
    if (searched_record.file_type == FT_REGULAR) {
        kprintf("sys_rmdir: %s is regular file!\n", pathname);
        return -1;
    }
    struct dir* dir = dir_open(cur_part, (uint32_t)inode_no);
    if (!dir_is_empty(dir)) {
        kprintf("sys_rmdir: %s is not empty\n", pathname);
        dir_close(dir);
        return -1;
    }
    int32_t ret = dir_remove(searched_record.parent_dir, dir);
    dir_close(dir);
    return ret;
}

static uint32_t get_parent_dir_inode_nr(uint32_t child_inode_nr, void* io_buf) {
    struct inode* child_dir_inode = inode_open(cur_part, child_inode_nr);
    uint32_t block_lba = child_dir_inode->i_sectors[0];
    inode_close(child_dir_inode);
    ide_read(cur_part->my_disk, block_lba, io_buf, 1);
    struct dir_entry* dir_e = (struct dir_entry*)io_buf;
    return dir_e[1].i_no;
}

static int get_child_dir_name(uint32_t p_inode_nr, uint32_t c_inode_nr, char* path, void* io_buf) {
    struct inode* parent_dir_inode = inode_open(cur_part, p_inode_nr);
    uint8_t block_idx = 0;
    uint32_t all_blocks[140] = {0};
    uint32_t block_cnt = 12;
    for (block_idx = 0; block_idx < 12; block_idx++) {
        all_blocks[block_idx] = parent_dir_inode->i_sectors[block_idx];
    }
    if (parent_dir_inode->i_sectors[12] != 0) {
        ide_read(cur_part->my_disk, parent_dir_inode->i_sectors[12], all_blocks + 12, 1);
        block_cnt = 140;
    }
    inode_close(parent_dir_inode);
    struct dir_entry* dir_e = (struct dir_entry*)io_buf;
    uint32_t dir_entry_size = cur_part->sb->dir_entry_size;
    uint32_t dir_entry_cnt = BLOCK_SIZE / dir_entry_size;
    for (block_idx = 0; block_idx < block_cnt; block_idx++) {
        if (all_blocks[block_idx] != 0) {
            ide_read(cur_part->my_disk, all_blocks[block_idx], io_buf, 1);
            uint32_t i = 0;
            for (i = 0; i < dir_entry_cnt; i++) {
                if ((dir_e + i)->i_no == c_inode_nr) {
                    strcat(path, "/");
                    strcat(path, (dir_e + i)->filename);
                    return 0;
                }
            }
        }
    }
    return -1;
}

char* sys_getcwd(char* buf, uint32_t size) {
    uint8_t* io_buf = (uint8_t*)get_kernel_pages(1);
    memset(io_buf, 0, PAGE_SIZE);
    uint32_t child_inode_nr = current_task->cwd_inode_nr;
    if (child_inode_nr == 0) {
        buf[0] = '/';
        buf[1] = 0;
        return buf;
    }
    memset(buf, 0, size);
    char full_path_reverse[MAX_PATH_LEN] = {0};
    while (child_inode_nr) {
        uint32_t parent_inode_nr = get_parent_dir_inode_nr(child_inode_nr, io_buf);
        if (get_child_dir_name(parent_inode_nr, child_inode_nr, full_path_reverse, io_buf) == -1) {
            return NULL;
        }
        child_inode_nr = parent_inode_nr;
    }
    char* last_slash;
    while ((last_slash = strrchr(full_path_reverse, '/'))) {
        uint32_t len = strlen(buf);
        strcpy(buf + len, last_slash);
        *last_slash = 0;
    }
    return buf;
}

int32_t sys_chdir(const char* path) {
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = search_file(path, &searched_record);
    if (inode_no == -1) {
        return -1;
    }
    if (searched_record.file_type != FT_DIRECTORY) {
        kprintf("sys_chdir: %s is not directory\n", path);
        return -1;
    }
    current_task->cwd_inode_nr = (uint32_t)inode_no;
    return 0;
}

int32_t sys_stat(const char* path, struct stat* buf) {
    if (!strcmp(path, ".") || !strcmp(path, "/.") || !strcmp(path, "/..")) {
        buf->st_filetype = FT_DIRECTORY;
        buf->st_ino = 0;
        buf->st_size = root_dir.inode->i_size;
        return 0;
    }
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    int inode_no = search_file(path, &searched_record);
    if (inode_no == -1) {
        kprintf("sys_stat: %s not found\n", path);
        return -1;
    }
    struct inode* obj_inode = inode_open(cur_part, (uint32_t)inode_no);
    buf->st_size = obj_inode->i_size;
    inode_close(obj_inode);
    buf->st_filetype = searched_record.file_type;
    buf->st_ino = (uint32_t)inode_no;
    return 0;
}
