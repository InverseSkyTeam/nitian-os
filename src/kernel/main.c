#include "./include/asmFunc.h"
#include "./include/assert.h"
#include "./initer/pic/pic.h"
#include "./initer/pit/pit.h"
#include "./initer/io/io.h"
#include "./initer/idt/idt.h"
#include "./lib/str/str.h"
#include "./memory/bitmap/bitmap.h"
#include "./memory/paging/paging.h"
#include "./memory/pool/pool.h"

struct BootInfo {
    uint8_t  cyls;
    uint8_t  leds;
    uint8_t  vmode;
    uint8_t  _pad;
    uint16_t scrnx;
    uint16_t scrny;
    uint32_t vram;
};

void KMain(void) {
    const struct BootInfo *bootInfo = (const struct BootInfo*)0x0FF0;
    initPalette();
    initIO((uint8_t*)bootInfo->vram, bootInfo->scrnx, bootInfo->scrny);
    initIDT();
    init_paging(bootInfo->vram);
    mm_init();

    setCursor(0, 0);

    setTextColor(14);
    printf("Kernel Inited.\n");

    setTextColor(10);
    printf("[OK] Paging enabled (identity 0-256MB + vram)\n");

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

    setTextColor(10);
    printf("[OK] Interrupts enabled, PIT timer running...\n");

    asm_sti();
    while(1) {
        asm_hlt();
    }
}
