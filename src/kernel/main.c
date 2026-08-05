#include "./include/asmFunc.h"
#include "./include/assert.h"
#include "./initer/pic/pic.h"
#include "./initer/pit/pit.h"
#include "./initer/io/io.h"
#include "./initer/idt/idt.h"
#include "./initer/gdt/gdt.h"
#include "./initer/tss/tss.h"
#include "./initer/idt/interrupt.h"
#include "./lib/str/str.h"
#include "./memory/bitmap/bitmap.h"
#include "./memory/pool/pool.h"
#include "./thread/thread.h"
#include "./thread/sync.h"
#include "./device/ioqueue.h"
#include "./device/keyboard.h"
#include "./device/ide.h"
#include "./userprog/process.h"
#include "./syscall/syscall.h"
#include "./lib/user/syscall.h"
#include "./lib/user/stdio.h"

struct BootInfo {
    uint8_t  cyls;
    uint8_t  leds;
    uint8_t  vmode;
    uint8_t  _pad;
    uint16_t scrnx;
    uint16_t scrny;
    uint32_t vram;
};

static struct ioqueue demo_ioq;
static volatile int g_produced = 0;
static volatile int g_consumed = 0;
#define DEMO_TOTAL 100

static void demo_producer(void* arg) {
    for (;;) {
        uint32_t old = asm_save_eflags();
        asm_cli();
        if (g_produced >= DEMO_TOTAL) {
            asm_restore_eflags(old);
            break;
        }
        char c = (char)('A' + (g_produced % 26));
        ioq_putchar(&demo_ioq, c);
        g_produced++;
        asm_restore_eflags(old);
        if (g_produced % 25 == 0) {
            setTextColor(13);
            printf("[P] produced %d chars\n", (int)g_produced);
        }
        thread_yield();
    }
    setTextColor(13);
    printf("[P] producer done (%d chars)\n", (int)g_produced);
}

static void demo_consumer(void* arg) {
    for (;;) {
        uint32_t old = asm_save_eflags();
        asm_cli();
        if (g_consumed >= DEMO_TOTAL) {
            asm_restore_eflags(old);
            break;
        }
        char c = ioq_getchar(&demo_ioq);
        g_consumed++;
        asm_restore_eflags(old);
        if (g_consumed % 25 == 0) {
            setTextColor(11);
            printf("[C] consumed %d chars (last='%c')\n", (int)g_consumed, c);
        }
        thread_yield();
    }
    setTextColor(11);
    printf("[C] consumer done (%d chars)\n", (int)g_consumed);
}

static void kbd_consumer(void* arg) {
    char line[80];
    int n = 0;
    for (;;) {
        uint32_t old = asm_save_eflags();
        asm_cli();
        char c = ioq_getchar(&keyboard_ioq);
        asm_restore_eflags(old);
        if (c == '\n' || c == '\r') {
            line[n] = 0;
            setTextColor(12);
            printf("[KBD] line: %s\n", line);
            n = 0;
        } else if (c == 0x08) {
            if (n > 0) {
                n--;
            }
        } else if (n < (int)sizeof(line) - 1) {
            line[n++] = c;
        }
        thread_yield();
    }
}

static int test_var_a = 0;
static int test_var_b = 0;

static void u_prog_a(void) {
    for (;;) {
        printf("%s%d%c", " program_a_pid:", getpid(), '\n');
    }
}

static void u_prog_b(void) {
    for (;;) {
        printf("%s%d%c", " program_b_pid:", getpid(), '\n');
    }
}

static void k_thread_a(void* arg) {
    for (;;) {
        if (g_tick % PIT_HZ == 0) {
            setTextColor(11);
            printf("thread_a_pid:0x%x user_var_a=%d\n", getpid(), test_var_a);
        }
        thread_yield();
    }
}

static void k_thread_b(void* arg) {
    for (;;) {
        if (g_tick % PIT_HZ == 0) {
            setTextColor(9);
            printf("thread_b_pid:0x%x user_var_b=%d\n", getpid(), test_var_b);
        }
        thread_yield();
    }
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
    printf("Kernel Inited.\n");

    setTextColor(10);
    printf("[OK] Higher Half Kernel @ 0xC0000000+\n");

    if (initPic() == 0) {
        printf("[OK] PIC inited\n");
    } else {
        setTextColor(12);
        printf("[FAIL] PIC init error\n");
    }

    initPIT(PIT_HZ);

    ASSERT(PIT_HZ == 100);

    char buf[64];
    strcpy(buf, "NiTianOS");
    ASSERT(strlen(buf) == 8);
    ASSERT(strcmp(buf, "NiTianOS") == 0);
    ASSERT(strcmp(buf, "NiTianOSx") < 0);
    strcat(buf, " v0.1");
    ASSERT(strlen(buf) == 13);
    printf("[OK] str: [%s] len=%d\n", buf, (int)strlen(buf));

    uint8_t pool[64];
    struct bitmap bm;
    bm.btmp_bytes_len = sizeof(pool);
    bm.bits = pool;
    bitmap_init(&bm);
    ASSERT(bitmap_scan_test(&bm, 0) == 0);
    int b0 = bitmap_scan(&bm, 3);
    ASSERT(b0 == 0);
    bitmap_set(&bm, 2, 1);
    ASSERT(bitmap_scan_test(&bm, 2) == 1);
    int b1 = bitmap_scan(&bm, 3);
    ASSERT(b1 == 3);
    memset(pool, 0xFF, sizeof(pool));
    ASSERT(bitmap_scan(&bm, 1) == -1);
    printf("[OK] bitmap: scan3=%d set2->scan3=%d full=-1\n", b0, b1);

    void* p1 = palloc(&kernel_pool);
    void* p2 = palloc(&kernel_pool);
    void* p3 = palloc(&kernel_pool);
    ASSERT(p1 != 0 && p2 != 0 && p3 != 0);
    ASSERT((uint32_t)p1 % PAGE_SIZE == 0);
    ASSERT(p1 != p2 && p2 != p3 && p1 != p3);
    printf("[OK] palloc: %x %x %x\n", (uint32_t)p1, (uint32_t)p2, (uint32_t)p3);
    uint32_t freed = (uint32_t)p2;
    pfree(&kernel_pool, freed);
    void* p4 = palloc(&kernel_pool);
    ASSERT((uint32_t)p4 == freed);
    printf("[OK] pfree/realloc: %x\n", (uint32_t)p4);

    ioq_init(&demo_ioq);
    keyboard_init();

    thread_init();
    setTextColor(10);
    printf("[OK] thread mgr ready, creating Ch.10 demo threads...\n");

    kernel_thread("producer", 3, demo_producer, 0);
    kernel_thread("consumer", 3, demo_consumer, 0);
    kernel_thread("kbd",      4, kbd_consumer, 0);

    process_execute((void*)u_prog_a, "u_prog_a");
    process_execute((void*)u_prog_b, "u_prog_b");
    kernel_thread("k_a", 4, k_thread_a, 0);
    kernel_thread("k_b", 4, k_thread_b, 0);

    setTextColor(10);
    printf("[OK] user processes + demo threads started, type to see [KBD] lines\n");

    setTextColor(14);
    printf("main_pid:0x%x\n", getpid());

    asm_sti();
    
    ide_init();

    for (;;) {
        thread_yield();
        asm_hlt();
    }
}
