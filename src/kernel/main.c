#include "./include/asmFunc.h"
#include "./include/assert.h"
#include "./initer/pic/pic.h"
#include "./initer/pit/pit.h"
#include "./initer/io/io.h"
#include "./initer/idt/idt.h"
#include "./initer/gdt/gdt.h"
#include "./initer/tss/tss.h"
#include "./memory/pool/pool.h"
#include "./thread/thread.h"
#include "./device/keyboard.h"
#include "./device/ide.h"
#include "./fs/fs.h"
#include "./userprog/process.h"
#include "./syscall/syscall.h"
#include "./shell/shell.h"
#include "./lib/user/syscall.h"
#include "./lib/user/stdio.h"

extern const unsigned char _binary_prog_no_arg_elf_start[];
extern const unsigned char _binary_prog_no_arg_elf_end[];
extern const unsigned char _binary_prog_arg_elf_start[];
extern const unsigned char _binary_prog_arg_elf_end[];
extern const unsigned char _binary_cat_elf_start[];
extern const unsigned char _binary_cat_elf_end[];
extern const unsigned char _binary_fork_demo_elf_start[];
extern const unsigned char _binary_fork_demo_elf_end[];
extern const unsigned char _binary_prog_pipe_elf_start[];
extern const unsigned char _binary_prog_pipe_elf_end[];
extern const unsigned char _binary_font_demo_elf_start[];
extern const unsigned char _binary_font_demo_elf_end[];
extern const unsigned char _binary_font_subset_ttf_start[];
extern const unsigned char _binary_font_subset_ttf_end[];

struct BootInfo {
    uint8_t  cyls;
    uint8_t  leds;
    uint8_t  vmode;
    uint8_t  _pad;
    uint16_t scrnx;
    uint16_t scrny;
    uint32_t vram;
};

static void k_thread_a(void* arg) {
    for (;;) {
        thread_yield();
    }
}

static void k_thread_b(void* arg) {
    for (;;) {
        thread_yield();
    }
}

static void init(void) {
    g_init_pid = getpid();
    uint32_t ret_pid = fork();
    if (ret_pid > 0) {
        for (;;) {
            int32_t status = 0;
            int32_t child_pid = wait(&status);
            if (child_pid != -1) {
                printf("init: reaped child %d, status %d\n", (int)child_pid, (int)status);
            } else {
                thread_yield();
            }
        }
    } else if (ret_pid == 0) {
        my_shell(NULL);
    } else {
        printf("init: fork failed\n");
        for (;;) {
        }
    }
}

static void write_prog(const char* name, const unsigned char* start, const unsigned char* end) {
    sys_unlink(name);
    int fd = open_file(name, O_CREAT | O_RDWR);
    if (fd == -1) {
        setTextColor(12);
        printf("[FAIL] write %s failed\n", name);
        return;
    }
    write_file(fd, (const void*)start, (uint32_t)(end - start));
    close_file(fd);
}

void KMain(void) {
    const struct BootInfo *bootInfo = (const struct BootInfo*)0x0FF0;
    initPalette();
    initIO((uint8_t*)bootInfo->vram, bootInfo->scrnx, bootInfo->scrny);
    initIDT();
    syscall_init();
    mm_init();
    gdt_init();
    tss_init();
    setTextColor(10);
    printf("[OK] TSS loaded, TR=0x%x esp0=0x%x\n", (uint32_t)asm_str(), tss.esp0);

    setCursor(0, 0);

    setTextColor(14);
    printf("NiTianOS Kernel Inited.\n");

    setTextColor(10);
    printf("[OK] Higher Half Kernel @ 0xC0000000+\n");

    if (initPic() == 0) {
        printf("[OK] PIC inited\n");
    } else {
        setTextColor(12);
        printf("[FAIL] PIC init error\n");
    }

    initPIT(PIT_HZ);

    keyboard_init();
    thread_init();
    setTextColor(10);
    printf("[OK] thread mgr ready\n");

    kernel_thread("k_a", 4, k_thread_a, 0);
    kernel_thread("k_b", 4, k_thread_b, 0);

    asm_sti();

    ide_init();
    filesys_init();

    write_prog("/prog_no_arg", _binary_prog_no_arg_elf_start, _binary_prog_no_arg_elf_end);
    write_prog("/prog_arg", _binary_prog_arg_elf_start, _binary_prog_arg_elf_end);
    write_prog("/cat", _binary_cat_elf_start, _binary_cat_elf_end);
    write_prog("/fork_demo", _binary_fork_demo_elf_start, _binary_fork_demo_elf_end);
    write_prog("/forktest", _binary_fork_demo_elf_start, _binary_fork_demo_elf_end);
    write_prog("/prog_pipe", _binary_prog_pipe_elf_start, _binary_prog_pipe_elf_end);
    write_prog("/font_demo", _binary_font_demo_elf_start, _binary_font_demo_elf_end);
    write_prog("/font.ttf", _binary_font_subset_ttf_start, _binary_font_subset_ttf_end);
    setTextColor(10);
    printf("[OK] user programs installed\n");

    process_execute(init, "init");
    for (;;) {
        thread_yield();
        asm_hlt();
    }
}
