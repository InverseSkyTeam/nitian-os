#include <stdint.h>

#define FONT_BASE ((const uint8_t*)0x11000)

void initIO(uint8_t* vram, int scrnx, int scrny);
void setTextColor(int color);
void setCursor(int x, int y);
int  getCursorX(void);
int  getCursorY(void);

void printf(const char* fmt, ...);

void showChar(uint8_t* vram, int pitch, int x, int y, int scrnx, int scrny, char c, int color, int bg);
void showString(uint8_t* vram, int pitch, int x, int y, int scrnx, int scrny, const char* s, int color, int bg);

void initPalette(void);
