// 参考: https://wiki.osdev.org/Assertions
#include "./include/assert.h"
#include "./initer/io/io.h"
#include "./include/asmFunc.h"

void assert_fail(const char* expr, const char* file, int line) {
    setTextColor(12);
    printf("\n*** ASSERT FAILED ***\n");
    printf("  expr: %s\n", expr);
    printf("  file: %s\n", file);
    printf("  line: %d\n", line);

    asm_cli();
    for (;;) {
        asm_hlt();
    }
}