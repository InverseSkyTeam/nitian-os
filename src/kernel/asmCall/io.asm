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
