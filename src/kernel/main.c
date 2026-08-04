#include "./include/asmFunc.h"
#include "./initer/pic/pic.h"
#include "./initer/io/io.h"

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
    io_init((uint8_t*)bootInfo->vram, bootInfo->scrnx, bootInfo->scrny);

    setCursor(0, 0);

    setTextColor(14);
    printf("Kernel Inited.");

    setTextColor(10);
    printf("\n");

    if (InitPic() == 0) {
        printf("[OK] PIC inited");
    } else {
        setTextColor(12); 
        printf("[FAIL] PIC init error");
    }

    asm_sti();
    while(1) {
        asm_hlt();
    }
}
