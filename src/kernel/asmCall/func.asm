bits 32
section .text

global asm_hlt
asm_hlt: hlt
        ret

global asm_cli
asm_cli: cli
        ret

global asm_sti
asm_sti: sti
        ret

global asm_stihlt
asm_stihlt: sti
            hlt
            ret