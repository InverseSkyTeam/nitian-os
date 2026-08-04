#include <stdint.h>

// BIOS 8x16 字体由 loader.asm 通过 INT 10h AH=11h 复制到此地址
// 256 字符 × 16 字节 = 4096 字节
#define FONT_BASE ((const uint8_t*)0x10000)

void showChar(uint8_t* vram, int pitch, int x, int y, int scrnx, int scrny, char c, int color, int bg);
void showString(uint8_t* vram, int pitch, int x, int y, int scrnx, int scrny, const char* s, int color, int bg);
void initPalette(void);