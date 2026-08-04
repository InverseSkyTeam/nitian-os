bits 32
section .text

extern KMain
extern __bss_start
extern __bss_end

global entry_start
entry_start:
        cli
        mov     edi, 0x400000
        mov     ecx, 1024
        xor     eax, eax
        rep     stosd

        mov     esi, 0x401000
        mov     edi, 0x400000
        xor     ebx, ebx
.pt_loop:
        mov     eax, esi
        or      eax, 3
        mov     [edi + ebx*4], eax
        push    edi
        push    ebx
        push    ecx
        mov     edi, esi
        mov     ecx, 1024
        mov     eax, ebx
        shl     eax, 22
.fill:
        mov     [edi], eax
        or      dword [edi], 7
        add     eax, 0x1000
        add     edi, 4
        loop    .fill
        pop     ecx
        pop     ebx
        pop     edi
        add     esi, 0x1000
        inc     ebx
        cmp     ebx, 64
        jne     .pt_loop

        mov     esi, 0x442000
        xor     ebx, ebx
.hi_loop:
        mov     eax, esi
        or      eax, 7
        mov     [0x400C00 + ebx*4], eax
        push    edi
        push    ebx
        push    ecx
        mov     edi, esi
        mov     ecx, 1024
        xor     eax, eax
.fill_hi:
        mov     [edi], eax
        or      dword [edi], 7
        add     eax, 0x1000
        add     edi, 4
        loop    .fill_hi
        pop     ecx
        pop     ebx
        pop     edi
        add     esi, 0x1000
        inc     ebx
        cmp     ebx, 4
        jne     .hi_loop

        mov     eax, [0x0FF8]
        shr     eax, 22
        mov     edx, 0x441000
        or      edx, 3
        mov     [0x400000 + eax*4], edx
        mov     eax, [0x0FF8]
        and     eax, 0xFFC00000
        mov     edi, 0x441000
        mov     ecx, 1024
.fill_vram:
        mov     [edi], eax
        or      dword [edi], 7
        add     eax, 0x1000
        add     edi, 4
        loop    .fill_vram

        mov     eax, 0x400000
        or      eax, 3
        mov     [0x400FFC], eax

        mov     eax, 0x400000
        mov     cr3, eax
        mov     eax, cr0
        or      eax, 0x80000000
        mov     cr0, eax
        jmp     .pg_on
.pg_on:

        mov     edi, __bss_start
        mov     ecx, __bss_end
        sub     ecx, __bss_start
        xor     eax, eax
        rep     stosb

        jmp     dword 0x10:KMain
