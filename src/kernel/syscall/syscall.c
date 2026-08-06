// 参考: 《操作系统真相还原》(于渊) 第12章 系统调用

#include "./syscall.h"
#include "../thread/thread.h"
#include "../initer/io/io.h"
#include "../lib/str/str.h"
#include "../include/assert.h"
#include "../include/asmFunc.h"
#include "../device/keyboard.h"
#include "../device/ioqueue.h"
#include "../fs/fs.h"
#include "../fs/file.h"
#include "../userprog/process.h"
#include "../userprog/exec.h"
#include "../userprog/fork.h"
#include "../userprog/wait_exit.h"
#include "../shell/pipe.h"

static uint32_t sys_getpid(void) {
    return current_task->pid;
}

static uint32_t sys_write(int32_t fd, char* str, uint32_t count) {
    if (fd < 0) {
        return (uint32_t)-1;
    }

    if (is_pipe(fd)) {
        return pipe_write(fd, str, count);
    }
    for (uint32_t i = 0; i < count; i++) {
        console_putc(str[i]);
    }
    return count;
}

static uint32_t sys_putchar(char c) {
    console_putc(c);
    return (uint32_t)(unsigned char)c;
}

static uint32_t sys_clear(void) {
    io_clear_screen();
    return 0;
}

static int32_t sys_read(int32_t fd, void* buf, uint32_t count) {
    if (fd == 1 || fd == 2) return -1;

    if (is_pipe(fd)) {
        return (int32_t)pipe_read(fd, buf, count);
    }
    if (fd == 0) {
        uint8_t* p = (uint8_t*)buf;
        uint32_t got = 0;
        asm_cli();
        while (got < count) {
            char c = ioq_getchar(&keyboard_ioq);
            asm_sti();
            p[got++] = (uint8_t)c;
            if (c == '\n' || c == '\r') break;
            asm_cli();
        }
        asm_sti();
        return (int32_t)got;
    }
    if (fd < 0 || fd >= (int32_t)MAX_FILES_OPEN_PER_PROC) return -1;
    if (count == 0) return 0;
    int32_t r = (int32_t)read_file(fd, buf, count);
    return r;
}

static const char* task_status_str(enum task_status s) {
    switch (s) {
    case TASK_RUNNING:  return "RUNNING";
    case TASK_READY:    return "READY";
    case TASK_BLOCKED:  return "BLOCKED";
    case TASK_WAITING:  return "WAITING";
    case TASK_HANGING:  return "HANGING";
    case TASK_DIED:     return "DIED";
    }
    return "?";
}

static int ps_action(struct task_struct* t, void* arg) {
    (void)arg;
    char buf[80];
    const char* parent = (t->parent_pid == -1) ? "(none)" : "?";
    if (t->parent_pid >= 0) {
        int n = 0;
        uint32_t v = (uint32_t)t->parent_pid;
        if (v == 0) { buf[n++] = '0'; }
        else {
            char tmp[12]; int m = 0;
            while (v) { tmp[m++] = (char)('0' + v % 10); v /= 10; }
            while (m--) buf[n++] = tmp[m];
        }
        buf[n] = 0;
        parent = buf;
    }
    kprintf("PID=%u PPID=%s STAT=%s TICKS=%u NAME=%s\n",
            t->pid, parent, task_status_str(t->status),
            t->elapsed_ticks, t->name);
    return 0;
}

static uint32_t sys_ps(void) {
    kprintf("=== ps ===\n");
    thread_traverse_all(ps_action, NULL);
    return 0;
}

uint32_t syscall_handler(struct Registers* r) {
    uint32_t nr = r->eax;
    switch (nr) {
    case SYS_GETPID:
        return sys_getpid();
    case SYS_WRITE:
        return sys_write((int32_t)r->ebx, (char*)r->ecx, (uint32_t)r->edx);
    case SYS_PUTCHAR:
        return sys_putchar((char)r->ebx);
    case SYS_CLEAR:
        return sys_clear();
    case SYS_READ:
        return (uint32_t)sys_read((int32_t)r->ebx, (void*)r->ecx, (uint32_t)r->edx);
    case SYS_FORK:
        return (uint32_t)sys_fork(r);
    case SYS_GETCWD:
        return (uint32_t)sys_getcwd((char*)r->ebx, (uint32_t)r->ecx);
    case SYS_CHDIR:
        return (uint32_t)sys_chdir((const char*)r->ebx);
    case SYS_MKDIR:
        return (uint32_t)sys_mkdir((const char*)r->ebx);
    case SYS_RMDIR:
        return (uint32_t)sys_rmdir((const char*)r->ebx);
    case SYS_OPEN:
        return (uint32_t)open_file((const char*)r->ebx, (uint8_t)r->ecx);
    case SYS_CLOSE:
        return (uint32_t)close_file((int)r->ebx);
    case SYS_LSEEK:
        return (uint32_t)sys_lseek((int32_t)r->ebx, (int32_t)r->ecx, (uint8_t)r->edx);
    case SYS_UNLINK:
        return (uint32_t)sys_unlink((const char*)r->ebx);
    case SYS_OPENDIR:
        return (uint32_t)sys_opendir((const char*)r->ebx);
    case SYS_CLOSEDIR:
        return (uint32_t)sys_closedir((struct dir*)r->ebx);
    case SYS_READDIR:
        return (uint32_t)sys_readdir((struct dir*)r->ebx);
    case SYS_REWINDDIR:
        sys_rewinddir((struct dir*)r->ebx);
        return 0;
    case SYS_STAT:
        return (uint32_t)sys_stat((const char*)r->ebx, (struct stat*)r->ecx);
    case SYS_PS:
        sys_ps();
        return 0;
    case SYS_EXECV:
        return (uint32_t)sys_execv((const char*)r->ebx, (const char**)r->ecx);
    case SYS_EXIT:
        sys_exit((int32_t)r->ebx);
        return 0;
    case SYS_WAIT:
        return (uint32_t)sys_wait((int32_t*)r->ebx);
    case SYS_PIPE:
        return (uint32_t)sys_pipe((int32_t*)r->ebx);
    case SYS_FD_REDIRECT:
        sys_fd_redirect((uint32_t)r->ebx, (uint32_t)r->ecx);
        return 0;
    default:
        return (uint32_t)-1;
    }
}

void syscall_init(void) {
    kprintf("[OK] syscall init, 0x80 registered (full table)\n");
}
