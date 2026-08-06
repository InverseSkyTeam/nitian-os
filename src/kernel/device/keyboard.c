// 参考: 《操作系统真相还原》(于渊) 第11章 输入输出系统
#include "./keyboard.h"
#include "../include/asmFunc.h"
#include "../thread/thread.h"

#define KEYBOARD_DATA 0x60

#define SC_SHIFT_L_DOWN 0x2A
#define SC_SHIFT_R_DOWN 0x36
#define SC_SHIFT_L_UP   0xAA
#define SC_SHIFT_R_UP   0xB6
#define SC_CAPS_DOWN    0x3A
#define SC_CTRL_L_DOWN  0x1D
#define SC_CTRL_L_UP    0x9D
#define SC_C_DOWN       0x2E                            

#define KBD_CHAR_CTRL_U (1)    
#define KBD_CHAR_CTRL_L (12)    

struct ioqueue keyboard_ioq;

static const char keymap[2][128] = {
    {
        0, 0x1b, '1', '2', '3', '4', '5', '6', '7', '8',
        '9', '0', '-', '=', 0x08, '\t',
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
        '[', ']', '\n', 0,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
        '\'', '`', 0, '\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
        0, '*', 0, ' ',
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    {
        0, 0x1b, '!', '@', '#', '$', '%', '^', '&', '*',
        '(', ')', '_', '+', 0x08, '\t',
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
        '{', '}', '\n', 0,
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
        '"', '~', 0, '|',
        'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
        0, '*', 0, ' ',
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
};

static uint8_t g_shift = 0;
static uint8_t g_caps = 0;
static uint8_t g_ctrl = 0;

void keyboard_init(void) {
    ioq_init(&keyboard_ioq);
}

void keyboard_handler(void) {
    uint8_t sc = inb(KEYBOARD_DATA);

    if (sc == SC_SHIFT_L_DOWN || sc == SC_SHIFT_R_DOWN) { g_shift = 1; return; }
    if (sc == SC_SHIFT_L_UP   || sc == SC_SHIFT_R_UP)   { g_shift = 0; return; }
    if (sc == SC_CAPS_DOWN) { g_caps = !g_caps; return; }
    if (sc == SC_CTRL_L_DOWN) { g_ctrl = 1; return; }
    if (sc == SC_CTRL_L_UP)   { g_ctrl = 0; return; }
    if (sc & 0x80) { return; }
    if (sc >= 128) { return; }

    if (g_ctrl && sc == SC_C_DOWN) {
        thread_kill_pid(g_foreground_pid);
        return;
    }

    if (g_ctrl && sc < 0x3b) {
        char c = 0;
        if (sc == 0x16) c = (char)KBD_CHAR_CTRL_U;      
        else if (sc == 0x26) c = (char)KBD_CHAR_CTRL_L;  
        if (c) {
            if (!ioq_full(&keyboard_ioq)) ioq_putchar(&keyboard_ioq, c);
            return;
        }
    }

    char c = keymap[g_shift ? 1 : 0][sc];
    if (g_caps && c >= 'a' && c <= 'z') c -= 32;

    if (c && !ioq_full(&keyboard_ioq)) {
        ioq_putchar(&keyboard_ioq, c);
    }
}
