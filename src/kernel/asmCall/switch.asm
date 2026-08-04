bits 32
section .text

global switch_to
switch_to:
    mov     eax, [esp+4]
    mov     edx, [esp+8]
    push    ebp
    push    ebx
    push    edi
    push    esi
    pushfd
    mov     [eax], esp
    mov     esp, [edx]
    popfd
    pop     esi
    pop     edi
    pop     ebx
    pop     ebp
    ret
