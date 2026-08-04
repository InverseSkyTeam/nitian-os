#include "./io.h"
#include "../../include/asmFunc.h"

void showChar(uint8_t* vram, int pitch, int x, int y, int scrnx, int scrny, char c, int color, int bg) {
    const uint8_t* font = FONT_BASE + ((uint8_t)c) * 16;

    for (int row = 0; row < 16; row++) {
        uint8_t bits = font[row];
        for (int col = 0; col < 8; col++) {
            int px = x + col;
            int py = y + row;
            if (px < 0 || px >= scrnx || py < 0 || py >= scrny) continue;

            if (bits & (0x80 >> col)) {
                vram[py * pitch + px] = (uint8_t)color;
            } else if (bg >= 0) {
                vram[py * pitch + px] = (uint8_t)bg;
            }
        }
    }
}

void showString(uint8_t* vram, int pitch, int x, int y, int scrnx, int scrny, const char* s, int color, int bg) {
    while (*s) {
        showChar(vram, pitch, x, y, scrnx, scrny, *s, color, bg);
        x += 8;
        s++;
    }
}

void initPalette(void) {
    static const uint8_t colors[16][3] = {
        {0, 0, 0},
        {0, 0, 170},
        {0, 170, 0},
        {0, 170, 170},
        {170, 0, 0},
        {170, 0, 170},
        {170, 85, 0},
        {170, 170, 170},
        {85, 85, 85},
        {85, 85, 255},
        {85, 255, 85},
        {85, 255, 255},
        {255, 85, 85},
        {255, 85, 255},
        {255, 255, 85},
        {255, 255, 255}
    };

    outb(0x03C8, 0);
    for (int i = 0; i < 16; i++) {
        outb(0x03C9, colors[i][0] >> 2);
        outb(0x03C9, colors[i][1] >> 2);
        outb(0x03C9, colors[i][2] >> 2);
    }
}
