// 参考: 《操作系统真相还原》(于渊) 第14章 文件系统
#ifndef FS_FS_H
#define FS_FS_H

#include <stdint.h>
#include "../device/ide.h"

#define MAX_FILES_PER_PART 4096
#define BITS_PER_SECTOR 4096
#define SECTOR_SIZE 512
#define BLOCK_SIZE SECTOR_SIZE
#define MAX_PATH_LEN 512

#define FS_MAGIC 0x19590318

enum file_types {
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
};

enum oflags {
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT = 4
};

enum whence {
    SEEK_SET = 1,
    SEEK_CUR,
    SEEK_END
};

struct path_search_record {
    char searched_path[MAX_PATH_LEN];
    struct dir* parent_dir;
    enum file_types file_type;
};

struct stat {
    uint32_t st_ino;
    uint32_t st_size;
    enum file_types st_filetype;
};

struct partition;
struct dir;
struct dir_entry;
extern struct partition* cur_part;

void filesys_init(void);

int32_t block_bitmap_alloc(struct partition* part);
void block_bitmap_free(struct partition* part, uint32_t lba);
int32_t inode_bitmap_alloc(struct partition* part);
void inode_bitmap_free(struct partition* part, uint32_t inode_no);

char* path_parse(char* pathname, char* name_store);
int search_dir_entry(struct partition* part, struct dir* pdir, const char* name, struct dir_entry* dir_e);
int create_dir_entry(struct partition* part, struct dir* pdir, uint32_t inode_no, const char* filename, enum file_types f_type);
int sync_dir_entry(struct dir* pdir, struct dir_entry* p_de, struct dir_entry* new_de);
int delete_dir_entry(struct partition* part, struct dir* pdir, uint32_t inode_no, void* io_buf);
int search_file(const char* pathname, struct path_search_record* searched_record);
int create_file(const char* pathname);
int open_file(const char* pathname, uint8_t flags);
int close_file(int fd);
uint32_t read_file(int fd, void* buf, uint32_t count);
uint32_t write_file(int fd, const void* buf, uint32_t count);
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);
int sys_unlink(const char* pathname);
int32_t sys_mkdir(const char* pathname);
struct dir* sys_opendir(const char* name);
int32_t sys_closedir(struct dir* dir);
struct dir_entry* sys_readdir(struct dir* dir);
void sys_rewinddir(struct dir* dir);
int32_t sys_rmdir(const char* pathname);
char* sys_getcwd(char* buf, uint32_t size);
int32_t sys_chdir(const char* path);
int32_t sys_stat(const char* path, struct stat* buf);

#endif
