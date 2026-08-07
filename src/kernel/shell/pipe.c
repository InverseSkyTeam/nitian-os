// 参考: 《操作系统真相还原》(于渊) 第15章 管道
#include "pipe.h"
#include "../fs/file.h"
#include "../thread/thread.h"
#include "../device/ioqueue.h"
#include "../memory/pool/pool.h"
#include "../include/asmFunc.h"

int32_t is_pipe(uint32_t local_fd) {
    uint32_t global_fd = fd_local2global(local_fd);
    if (global_fd >= MAX_FILE_OPEN) {
        return 0;
    }
    return file_table[global_fd].fd_flag == PIPE_FLAG;
}

int32_t sys_pipe(int32_t pipefd[2]) {

    int32_t global_fd = -1;
    for (uint32_t i = 3; i < MAX_FILE_OPEN; i++) {
        if (file_table[i].fd_inode == NULL) {
            global_fd = (int32_t)i;
            break;
        }
    }
    if (global_fd == -1) {
        return -1;
    }

    void* buf = get_kernel_pages(1);
    if (buf == NULL) {
        return -1;
    }

    ioq_init((struct ioqueue*)buf);

    file_table[global_fd].fd_inode = (struct inode*)buf;
    file_table[global_fd].fd_flag = PIPE_FLAG;
    file_table[global_fd].fd_pos = 2;

    pipefd[0] = fd_install(global_fd);
    pipefd[1] = fd_install(global_fd);
    if (pipefd[0] == -1 || pipefd[1] == -1) {
        if (pipefd[0] != -1) fd_release((uint32_t)pipefd[0]);
        if (pipefd[1] != -1) fd_release((uint32_t)pipefd[1]);
        free_kernel_page((uint32_t)buf);
        file_table[global_fd].fd_inode = NULL;
        file_table[global_fd].fd_flag = 0;
        file_table[global_fd].fd_pos = 0;
        return -1;
    }
    return 0;
}

uint32_t pipe_read(int32_t fd, void* buf, uint32_t count) {
    uint32_t global_fd = fd_local2global(fd);
    if (global_fd >= MAX_FILE_OPEN || file_table[global_fd].fd_inode == NULL) {
        return 0;
    }
    struct ioqueue* ioq = (struct ioqueue*)file_table[global_fd].fd_inode;

    uint32_t ioq_len = ioq_length(ioq);
    uint32_t size = (ioq_len > count) ? count : ioq_len;
    char* buffer = (char*)buf;
    uint32_t bytes_read = 0;

    asm_cli();
    while (bytes_read < size) {
        buffer[bytes_read] = ioq_getchar(ioq);
        ++bytes_read;
    }
    asm_sti();
    return bytes_read;
}

uint32_t pipe_write(int32_t fd, const void* buf, uint32_t count) {
    uint32_t global_fd = fd_local2global(fd);
    if (global_fd >= MAX_FILE_OPEN || file_table[global_fd].fd_inode == NULL) {
        return 0;
    }
    struct ioqueue* ioq = (struct ioqueue*)file_table[global_fd].fd_inode;

    uint32_t ioq_left = BUFSIZE - ioq_length(ioq);
    uint32_t size = (ioq_left > count) ? count : ioq_left;

    const char* buffer = (const char*)buf;
    uint32_t bytes_write = 0;

    asm_cli();
    while (bytes_write < size) {
        ioq_putchar(ioq, buffer[bytes_write]);
        ++bytes_write;
    }
    asm_sti();
    return bytes_write;
}

void sys_fd_redirect(uint32_t old_local_fd, uint32_t new_local_fd) {
    struct task_struct* cur = current_task;
    if (new_local_fd < 3) {

        cur->fd_table[old_local_fd] = new_local_fd;
    } else {
        uint32_t new_global_fd = cur->fd_table[new_local_fd];
        cur->fd_table[old_local_fd] = new_global_fd;
    }
}
