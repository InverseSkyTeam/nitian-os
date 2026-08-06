#include "./io.h"
#include "../../include/asmFunc.h"
#include <stdarg.h>

static uint8_t* g_vram      = (uint8_t*)0;
static int      g_scrnx     = 0;
static int      g_scrny     = 0;
static int      g_pitch     = 0;
static int      g_cursor_x  = 0;
static int      g_cursor_y  = -20;
static int      g_text_color = 7;

#define PRINTF_LINE_GAP 20

void initIO(uint8_t* vram, int scrnx, int scrny) {
    g_vram      = vram;
    g_scrnx     = scrnx;
    g_scrny     = scrny;
    g_pitch     = scrnx;
    g_cursor_x  = 0;
    g_cursor_y  = 0;
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

#define DEBUG_CONSOLE_PORT 0xE9

static void vram_shift_up(int line_bytes) {
    int total_bytes = g_scrny * g_pitch;
    if ((g_pitch & 3) == 0 && ((uintptr_t)g_vram & 3) == 0) {
        uint32_t* dw = (uint32_t*)g_vram;
        int line_dw = line_bytes / 4;
        int total_dw = total_bytes / 4;
        for (int i = line_dw; i < total_dw; i++) dw[i - line_dw] = dw[i];
        for (int i = total_dw - line_dw; i < total_dw; i++) dw[i] = 0;
    } else {
        for (int i = line_bytes; i < total_bytes; i++) g_vram[i - line_bytes] = g_vram[i];
        for (int i = total_bytes - line_bytes; i < total_bytes; i++) g_vram[i] = 0;
    }
}

static void vram_zero_all(void) {
    int total_bytes = g_scrny * g_pitch;
    if ((g_pitch & 3) == 0 && ((uintptr_t)g_vram & 3) == 0) {
        uint32_t* dw = (uint32_t*)g_vram;
        int total_dw = total_bytes / 4;
        for (int i = 0; i < total_dw; i++) dw[i] = 0;
    } else {
        for (int i = 0; i < total_bytes; i++) g_vram[i] = 0;
    }
}

static void scroll_screen(void) {
    if (g_vram == 0 || g_scrnx <= 0 || g_scrny <= 0 || g_pitch <= 0) return;
    vram_shift_up(PRINTF_LINE_GAP * g_pitch);
    g_cursor_y -= PRINTF_LINE_GAP;
}

static void putc(char c) {
    if (c == '\n') {
        g_cursor_y += PRINTF_LINE_GAP;
        g_cursor_x = 0;
        if (g_cursor_y + PRINTF_LINE_GAP > g_scrny) {
            scroll_screen();
        }
    } else {

        showChar(g_vram, g_pitch,
                 g_cursor_x, g_cursor_y,
                 g_scrnx, g_scrny,
                 c, g_text_color, 0);
        g_cursor_x += 8;

        if (g_cursor_x + 8 > g_scrnx) {
            g_cursor_y += PRINTF_LINE_GAP;
            g_cursor_x = 0;
            if (g_cursor_y + PRINTF_LINE_GAP > g_scrny) {
                scroll_screen();
            }
        }
    }
    outb(DEBUG_CONSOLE_PORT, (uint8_t)c);
}

static void printUnsigned(uint32_t v, int base, int upper,
                          int width, int pad0, int hexPrefix) {
    static const char lo[] = "0123456789abcdef";
    static const char up[] = "0123456789ABCDEF";
    const char* digits = upper ? up : lo;
    char buf[33];
    int n = 0;

    if (v == 0) {
        buf[n++] = '0';
    } else {
        while (v) {
            buf[n++] = digits[v % base];
            v /= base;
        }
    }

    int body = (hexPrefix ? 2 : 0) + n;
    int pad  = (width > body) ? (width - body) : 0;

    if (pad0) {
        if (hexPrefix) { putc('0'); putc(upper ? 'X' : 'x'); }
        while (pad--) putc('0');
    } else {
        while (pad--) putc(' ');
        if (hexPrefix) { putc('0'); putc(upper ? 'X' : 'x'); }
    }
    while (n--) putc(buf[n]);
}

static void printSigned(int v, int width, int pad0) {
    unsigned int uv = (unsigned int)v;
    int neg = 0;
    char buf[12];
    int n = 0;

    if (v < 0) { neg = 1; uv = 0u - uv; }
    if (uv == 0) {
        buf[n++] = '0';
    } else {
        while (uv) { buf[n++] = '0' + uv % 10; uv /= 10; }
    }

    int body = neg + n;
    int pad  = (width > body) ? (width - body) : 0;

    if (pad0) {
        if (neg) putc('-');
        while (pad--) putc('0');
    } else {
        while (pad--) putc(' ');
        if (neg) putc('-');
    }
    while (n--) putc(buf[n]);
}

void kprintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            putc(*fmt);
            continue;
        }
        ++fmt;

        int pad0 = 0, hexPre = 0, width = 0;
        for (;;) {
            if (*fmt == '0')      { pad0 = 1;  ++fmt; }
            else if (*fmt == '#') { hexPre = 1; ++fmt; }
            else break;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            ++fmt;
        }
        if (*fmt == 'l') ++fmt;

        switch (*fmt) {
        case 'd': printSigned(va_arg(ap, int), width, pad0); break;
        case 'u': printUnsigned(va_arg(ap, unsigned int), 10, 0, width, pad0, 0); break;
        case 'x': printUnsigned(va_arg(ap, unsigned int), 16, 0, width, pad0, hexPre); break;
        case 'X': printUnsigned(va_arg(ap, unsigned int), 16, 1, width, pad0, hexPre); break;
        case 'o': printUnsigned(va_arg(ap, unsigned int), 8, 0, width, pad0, 0); break;
        case 'c': putc((char)va_arg(ap, int)); break;
        case 's': {
            const char* s = va_arg(ap, const char*);
            if (!s) s = "(null)";
            while (*s) putc(*s++);
            break;
        }
        case '%': putc('%'); break;
        case '\0': --fmt; break;
        default:  putc('%'); putc(*fmt); break;
        }
    }
    va_end(ap);
}

void console_putc(char c) {
    putc(c);
}

void console_put_str(const char* s) {
    while (*s) {
        putc(*s++);
    }
}

void io_clear_screen(void) {
    vram_zero_all();
    g_cursor_x = 0;
    g_cursor_y = 0;
    g_text_color = 7;
}

void showChar(uint8_t* vram, int pitch, int x, int y, int scrnx, int scrny, char c, int color, int bg) {    const uint8_t* font = FONT_BASE + ((uint8_t)c) * 16;

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
