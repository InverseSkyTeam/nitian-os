#include "./include/asmFunc.h"
#include "./initer/pic/pic.h"

void KMain(void) {
    InitPic();
    
    asm_sti();

    while(1) {
        asm_hlt();
    }
}