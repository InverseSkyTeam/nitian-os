// 参考: https://wiki.osdev.org/Assertions
#include "./include/assert.h"
#include "./initer/io/io.h"
#include "./include/asmFunc.h"

void assert_fail(const char* expr, const char* file, int line) {
    setTextColor(12);
    kprintf("\n*** ASSERT FAILED ***\n");
    kprintf("  expr: %s\n", expr);
    kprintf("  file: %s\n", file);
    kprintf("  line: %d\n", line);

    asm_cli();
    for (;;) {
        asm_hlt();
    }
}