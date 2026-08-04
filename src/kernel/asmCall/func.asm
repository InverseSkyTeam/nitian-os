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

global asm_read_cr0
asm_read_cr0:
        mov     eax, cr0
        ret

global asm_write_cr0
asm_write_cr0:
        mov     eax, [esp+4]
        mov     cr0, eax
        jmp     .flush
.flush:
        ret

global asm_write_cr3
asm_write_cr3:
        mov     eax, [esp+4]
        mov     cr3, eax
        ret

global asm_save_eflags
asm_save_eflags:
        pushfd
        pop     eax
        ret

global asm_restore_eflags
asm_restore_eflags:
        mov     eax, [esp+4]
        push    eax
        popfd
        ret

global asm_lgdt
asm_lgdt:
        mov     eax, [esp+4]
        lgdt    [eax]
        ret

global asm_ltr
asm_ltr:
        mov     ax, [esp+4]
        ltr     ax
        ret

global asm_str
asm_str:
        str     eax
        ret