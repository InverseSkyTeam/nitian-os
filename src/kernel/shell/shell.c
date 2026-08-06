// 参考: 《操作系统真相还原》(于渊) 第15章 系统交互
#include "./shell.h"
#include "./buildin_cmd.h"
#include "../lib/user/syscall.h"
#include "../lib/user/stdio.h"
#include "../lib/str/str.h"
#include "../fs/fs.h"
#include "../thread/thread.h"
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

static void cmd_execute(int32_t argc, char** argv) {
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

        int32_t pid = fork();
        if (pid > 0) {
            g_foreground_pid = (uint32_t)pid;                            
            int32_t status = 0;
            int32_t child_pid = wait(&status);
            g_foreground_pid = (uint32_t)-1;                      
            printf("\n[prog %d exited, status %d]\n", (int)child_pid, (int)status);
        } else if (pid == 0) {
            make_clear_abs_path(argv[0], final_path);
            argv[0] = final_path;
            struct stat file_stat;
            memset(&file_stat, 0, sizeof(struct stat));
            if (stat(argv[0], &file_stat) == -1) {
                printf("my_shell: cannot access %s: No such file or directory\n", argv[0]);
                exit(-1);
            }
            execv(argv[0], (const char**)argv);
            printf("execv %s failed.\n", argv[0]);
            exit(-1);
        } else {
            printf("fork failed.\n");
        }
    }
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

        char* pipe_symbol = strchr(cmd_line, '|');
        if (pipe_symbol) {
            int32_t fd[2] = { -1, -1 };
            if (pipe(fd) == -1) {
                printf("my_shell: pipe create failed.\n");
                continue;
            }

            fd_redirect(1, (uint32_t)fd[1]);
            char* each_cmd = cmd_line;
            pipe_symbol = strchr(each_cmd, '|');
            *pipe_symbol = 0;
            argc = -1;
            argc = cmd_parse(each_cmd, argv, ' ');
            cmd_execute(argc, argv);

            each_cmd = pipe_symbol + 1;
            fd_redirect(0, (uint32_t)fd[0]);
            while ((pipe_symbol = strchr(each_cmd, '|'))) {
                *pipe_symbol = 0;
                argc = -1;
                argc = cmd_parse(each_cmd, argv, ' ');
                cmd_execute(argc, argv);
                each_cmd = pipe_symbol + 1;
            }

            fd_redirect(1, 1);
            argc = -1;
            argc = cmd_parse(each_cmd, argv, ' ');
            cmd_execute(argc, argv);

            fd_redirect(0, 0);
            close(fd[0]);
            close(fd[1]);
        } else {
            argc = cmd_parse(cmd_line, argv, ' ');
            if (argc == -1) {
                printf("num of arguments exceed %d\n", MAX_ARG_NR);
                continue;
            }
            cmd_execute(argc, argv);
        }
    }
}
