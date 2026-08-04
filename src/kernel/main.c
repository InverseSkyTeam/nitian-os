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

    showString((uint8_t*)bootInfo->vram, (int)bootInfo->scrnx, 50, 50, bootInfo->scrnx, bootInfo->scrny, "Hello, OS!", 7, -1);

    InitPic();

    asm_sti();

    while(1) {
        asm_hlt();
    }
}
