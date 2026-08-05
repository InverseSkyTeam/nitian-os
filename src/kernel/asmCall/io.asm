[bits 32]
section .text

global inb
inb:
    mov dx, [esp + 4]
    in al, dx
    ret

global outb
outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global insw
insw:
    mov dx, [esp + 4]
    mov edi, [esp + 8]
    mov ecx, [esp + 12]
    cld
    rep insw
    ret

global outsw
outsw:
    mov dx, [esp + 4]
    mov esi, [esp + 8]
    mov ecx, [esp + 12]
    cld
    rep outsw
    ret
