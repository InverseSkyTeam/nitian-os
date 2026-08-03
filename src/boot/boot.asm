CYLS    equ     31
        org     0x7C00

        jmp     entry
        nop
        db      "NITIANOS"      
        dw      512             
        db      1               
        dw      1               
        db      2               
        dw      224             
        dw      2880            
        db      0xF0            
        dw      9               
        dw      18              
        dw      2               
        dd      0               
        dd      2880            
        db      0, 0, 0x29      
        dd      0xFFFFFFFF      
        db      "NITIANOS"   
        db      "FAT12   "      
        times   18 db 0         

entry:
        mov     ax, 0
        mov     ss, ax
        mov     sp, 0x7C00
        mov     ds, ax

        mov     ax, 0x0800      
        mov     es, ax
        mov     ch, 0           
        mov     dh, 0           
        mov     cl, 1           

readloop:
        mov     si, 0           

retry:
        mov     ah, 0x02        
        mov     al, 1           
        mov     bx, 0
        mov     dl, 0x00        
        int     0x13
        jnc     next
        add     si, 1
        cmp     si, 5
        jae     error
        mov     ah, 0x00
        mov     dl, 0x00
        int     0x13            
        jmp     retry

next:
        mov     ax, es
        add     ax, 0x0020      
        mov     es, ax
        add     cl, 1
        cmp     cl, 18
        jbe     readloop
        mov     cl, 1
        add     dh, 1
        cmp     dh, 2
        jb      readloop
        mov     dh, 0
        add     ch, 1
        cmp     ch, CYLS
        jb      readloop

        mov     [0x0FF0], ch   
        jmp     0xC200

error:
        mov     si, msg

putloop:
        mov     al, [si]
        add     si, 1
        cmp     al, 0
        je      fin
        mov     ah, 0x0E
        mov     bx, 15
        int     0x10
        jmp     putloop

fin:
        hlt
        jmp     fin

msg:
        db      0x0A, 0x0A
        db      "load error"
        db      0x0A
        db      0

        times   510-($-$$) db 0
        db      0x55, 0xAA