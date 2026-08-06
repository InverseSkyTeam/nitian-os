// 参考: 《操作系统真相还原》(于渊) 第15章 系统交互
#include "./shell.h"
#include "./buildin_cmd.h"
#include "../lib/user/syscall.h"
#include "../lib/user/stdio.h"
#include "../lib/str/str.h"
#include "../fs/fs.h"
#include "../initer/io/io.h"
#include "../include/assert.h"

#define MAX_ARG_NR 16

#define KBD_CTRL_U 0x01
#define KBD_CTRL_L 0x0C

static char cmd_line[MAX_PATH_LEN] = {0};

char final_path[MAX_PATH_LEN] = {0};

static char cwd_cache[64] = {0};

char* argv[MAX_ARG_NR];
static int32_t argc = -1;

void print_prompt(void) {
    printf("[nitian@nitian-os %s]$ ", cwd_cache);
}

static void shell_erase(void) {
    int x = getCursorX();
    int y = getCursorY();
    if (x <= 0) return;
    setCursor(x - 8, y);
    console_putc(' ');
    setCursor(x - 8, y);
}

static void readline(char* buf, int32_t count) {
    ASSERT(buf != NULL && count > 0);
    char* pos = buf;
    while (read(0, pos, 1) != -1 && (pos - buf) < count) {
        switch (*pos) {
        case '\n':
        case '\r':
            *pos = 0;
            putchar('\n');
            return;
        case '\b':
            if (pos > buf) {
                shell_erase();
                --pos;
                *pos = 0;
            }
            break;
        case KBD_CTRL_L:
            *pos = 0;
            clear();
            print_prompt();
            printf("%s", buf);
            break;
        case KBD_CTRL_U:
            while (pos > buf) {
                shell_erase();
                --pos;
            }
            *pos = 0;
            break;
        default:
            putchar(*pos);
            ++pos;
        }
    }
    printf("readline: can't find enter_key, max %d chars\n", count - 1);
}

static int32_t cmd_parse(char* cmd_str, char** argv, char token) {
    ASSERT(cmd_str != NULL);
    int32_t arg_idx = 0;
    while (arg_idx < MAX_ARG_NR) argv[arg_idx++] = NULL;

    char* next = cmd_str;
    int32_t argc = 0;
    while (*next) {
        while (*next == token) ++next;
        if (*next == 0) break;
        if (argc >= MAX_ARG_NR) return -1;
        argv[argc] = next;
        while (*next && *next != token) ++next;
        if (*next) *next++ = 0;
        ++argc;
    }
    return argc;
}

void my_shell(void* arg) {
    (void)arg;
    clear();
    cwd_cache[0] = '/';
    cwd_cache[1] = 0;
    for (;;) {
        print_prompt();
        memset(final_path, 0, MAX_PATH_LEN);
        memset(cmd_line, 0, MAX_PATH_LEN);
        readline(cmd_line, MAX_PATH_LEN);
        if (cmd_line[0] == 0) continue;

        argc = cmd_parse(cmd_line, argv, ' ');
        if (argc == -1) {
            printf("num of arguments exceed %d\n", MAX_ARG_NR);
            continue;
        }

        if (!strcmp("ls", argv[0])) {
            buildin_ls(argc, argv);
        } else if (!strcmp("cd", argv[0])) {
            if (buildin_cd(argc, argv) != NULL) {
                memset(cwd_cache, 0, sizeof(cwd_cache));
                strcpy(cwd_cache, final_path);
            }
        } else if (!strcmp("pwd", argv[0])) {
            buildin_pwd(argc, argv);
        } else if (!strcmp("ps", argv[0])) {
            buildin_ps(argc, argv);
        } else if (!strcmp("clear", argv[0])) {
            buildin_clear(argc, argv);
        } else if (!strcmp("mkdir", argv[0])) {
            buildin_mkdir(argc, argv);
        } else if (!strcmp("rmdir", argv[0])) {
            buildin_rmdir(argc, argv);
        } else if (!strcmp("rm", argv[0])) {
            buildin_rm(argc, argv);
        } else {
            printf("external command: %s\n", argv[0]);
        }
    }
}
