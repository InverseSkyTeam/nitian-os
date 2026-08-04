#include "./include/asmFunc.h"
#include "./include/assert.h"
#include "./initer/pic/pic.h"
#include "./initer/pit/pit.h"
#include "./initer/io/io.h"
#include "./initer/idt/idt.h"

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

    setCursor(0, 0);

    setTextColor(14);
    printf("Kernel Inited.\n");

    setTextColor(10);

    if (initPic() == 0) {
        printf("[OK] PIC inited\n");
    } else {
        setTextColor(12); 
        printf("[FAIL] PIC init error\n");
    }

    initPIT(PIT_HZ);

    ASSERT(PIT_HZ == 100);

    setTextColor(10);
    printf("[OK] Interrupts enabled, PIT timer running...\n");

    asm_sti();
    while(1) {
        asm_hlt();
    }
}
