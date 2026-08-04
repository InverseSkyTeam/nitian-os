#include "./io.h"
#include "../../include/asmFunc.h"

static uint8_t* g_vram      = (uint8_t*)0;
static int      g_scrnx     = 0;
static int      g_scrny     = 0;
static int      g_pitch     = 0;
static int      g_cursor_x  = 0;
static int      g_cursor_y  = -20;  
static int      g_text_color = 7; 

#define PRINTF_LINE_GAP 20

void io_init(uint8_t* vram, int scrnx, int scrny) {
    g_vram      = vram;
    g_scrnx     = scrnx;
    g_scrny     = scrny;
    g_pitch     = scrnx;  
    g_cursor_x  = 0;
    g_cursor_y  = -20;
    g_text_color = 7;
}

void setTextColor(int color) {
    g_text_color = color & 0xFF;
}

void setCursor(int x, int y) {
    g_cursor_x = x;
    g_cursor_y = y;
}

int getCursorX(void) { return g_cursor_x; }
int getCursorY(void) { return g_cursor_y; }

void printf(const char* s) {
    while (*s) {
        if (*s == '\n') {
            g_cursor_y += PRINTF_LINE_GAP;
            g_cursor_x = 0;
        } else {
            showChar(g_vram, g_pitch,
                     g_cursor_x, g_cursor_y,
                     g_scrnx, g_scrny,
                     *s, g_text_color, -1);
            g_cursor_x += 8;

            if (g_cursor_x + 8 > g_scrnx) {
                g_cursor_y += PRINTF_LINE_GAP;
                g_cursor_x = 0;
            }
        }
        s++;
    }
}

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
