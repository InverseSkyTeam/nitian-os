KERNEL  equ     0x00280000
DSKCAC  equ     0x00100000
DSKCAC0 equ     0x00008000
VBEMODE equ     0x105
VBEINFO equ     0x0FF0 - 256

CYLS    equ     0x0FF0
LEDS    equ     0x0FF1
VMODE   equ     0x0FF2
SCRNX   equ     0x0FF4
SCRNY   equ     0x0FF6
VRAM    equ     0x0FF8

STACK_PHYS equ  0x00090000

        org     0xC200

kernel_addr:    dd      0

        mov     ax, 0x9000
        mov     es, ax
        mov     di, 0
        mov     cx, VBEMODE
        mov     ax, 0x4F01
        int     0x10
        cmp     ax, 0x004F
        jne     vbe_fail

        mov     bx, VBEMODE + 0x4000
        mov     ax, 0x4F02
        int     0x10
        cmp     ax, 0x004F
        jne     vbe_fail

        mov     byte [VMODE], 8
        mov     ax, [es:0x0012]
        mov     [SCRNX], ax
        mov     ax, [es:0x0014]
        mov     [SCRNY], ax
        mov     eax, [es:0x0028]
        mov     [VRAM], eax
        jmp     vbe_done

vbe_fail:
        mov     al, 0x13
        mov     ah, 0x00
        int     0x10
        mov     byte [VMODE], 8
        mov     word [SCRNX], 320
        mov     word [SCRNY], 200
        mov     dword [VRAM], 0x000A0000

vbe_done:
        mov     ax, 0x1130
        mov     bh, 0x06
        int     0x10

        push    ds
        push    es
        push    si
        push    di
        push    cx

        mov     ax, es
        mov     si, bp

        mov     bx, 0x9C00
        mov     es, bx
        xor     di, di

        mov     ds, ax

        mov     cx, 256 * 16
        rep     movsb

        pop     cx
        pop     di
        pop     si
        pop     es
        pop     ds

        mov     ah, 0x02
        int     0x16
        mov     [LEDS], al

        mov     dword [0x6000], 0
        mov     edi, 0x6004

.e820_loop:
        mov     eax, 0xE820
        mov     edx, 0x534D4150
        mov     ecx, 24
        int     0x15
        jc      .e820_done
        cmp     eax, 0x534D4150
        jne     .e820_done
        add     edi, 24
        inc     dword [0x6000]
        cmp     ebx, 0
        je      .e820_done
        jmp     .e820_loop

.e820_done:
        mov     al, 0xFF
        out     0x21, al
        nop
        out     0xA1, al

        cli

        call    waitkbdout
        mov     al, 0xD1
        out     0x64, al
        call    waitkbdout
        mov     al, 0xDF
        out     0x60, al
        call    waitkbdout

        lgdt    [GDTR0]
        lidt    [IDTR0]
        mov     eax, cr0
        and     eax, 0x7FFFFFFF
        or      eax, 0x00000001
        mov     cr0, eax
        jmp     pipelineflush

pipelineflush:
        mov     ax, 1*8
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        mov     ss, ax

        mov     eax, cr4
        or      eax, 0x600
        mov     cr4, eax

        mov     esi, [kernel_addr]
        mov     edi, KERNEL
        mov     ecx, 512*1024/4
        call    memcpy

        mov     esi, 0x7C00
        mov     edi, DSKCAC
        mov     ecx, 512/4
        call    memcpy

        mov     esi, DSKCAC0
        mov     edi, DSKCAC+512
        mov     ecx, 0
        mov     cl, byte [CYLS]
        imul    ecx, 512*18*2/4
        sub     ecx, 512/4
        call    memcpy

        mov     esp, STACK_PHYS

        jmp     DWORD 2*8:KERNEL

waitkbdout:
        in      al, 0x64
        and     al, 0x02
        jnz     waitkbdout
        ret

memcpy:
        mov     eax, [esi]
        add     esi, 4
        mov     [edi], eax
        add     edi, 4
        sub     ecx, 1
        jnz     memcpy
        ret

        alignb  16

IDT0:
    %rep 256
        dw  default_handler
        dw  0x08
        db  0
        db  0x8E
        dw  0
    %endrep

IDTR0:
    dw  256*8 - 1
    dd  IDT0

default_handler:
    iret

GDT0:
    db 0, 0, 0, 0, 0, 0, 0, 0
    dw 0xFFFF, 0x0000, 0x9200, 0x00CF
    dw 0xFFFF, 0x0000, 0x9A00, 0x00CF

        dw      0
GDTR0:
        dw      8*3-1
        dd      GDT0

        alignb  16