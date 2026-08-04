// 参考: 《操作系统真相还原》(于渊) 第12章 格式化输出

#include "./stdio.h"
#include "./syscall.h"

static void itoa(uint32_t value, char** buf_ptr_addr, uint8_t base) {
    uint32_t m = value % base;
    uint32_t i = value / base;
    if (i) {
        itoa(i, buf_ptr_addr, base);
    }
    if (m < 10) {
        *((*buf_ptr_addr)++) = (char)(m + '0');
    } else {
        *((*buf_ptr_addr)++) = (char)(m - 10 + 'A');
    }
}

uint32_t vsprintf(char* str, const char* format, va_list ap) {
    char* buf_ptr = str;
    const char* fmt = format;
    char ch;

    while ((ch = *fmt) != 0) {
        if (ch != '%') {
            *buf_ptr++ = ch;
            fmt++;
            continue;
        }
        
        fmt++;
        ch = *fmt;
        switch (ch) {
        case 'x': {
            
            uint32_t v = (uint32_t)va_arg(ap, int);
            itoa(v, &buf_ptr, 16);
            break;
        }
        case 'd': {
            
            int32_t v = va_arg(ap, int);
            if (v < 0) {
                *buf_ptr++ = '-';
                v = (int32_t)(0u - (uint32_t)v);
            }
            itoa((uint32_t)v, &buf_ptr, 10);
            break;
        }
        case 'c':
            *buf_ptr++ = (char)va_arg(ap, int);
            break;
        case 's': {
            const char* s = va_arg(ap, const char*);
            if (s == 0) {
                s = "(null)";
            }
            while (*s) {
                *buf_ptr++ = *s++;
            }
            break;
        }
        case '%':
            *buf_ptr++ = '%';
            break;
        default:
            *buf_ptr++ = '%';
            *buf_ptr++ = ch;
            break;
        }
        if (ch) {
            fmt++;
        }
    }
    *buf_ptr = 0;
    return (uint32_t)(buf_ptr - str);
}

void printf(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    char buf[1024] = {0};
    vsprintf(buf, format, ap);
    va_end(ap);
    write(buf);
}

uint32_t sprintf(char* buf, const char* format, ...) {
    va_list ap;
    uint32_t retval;
    va_start(ap, format);
    retval = vsprintf(buf, format, ap);
    va_end(ap);
    return retval;
}
