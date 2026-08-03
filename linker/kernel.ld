OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(KMain)                      

_kernel_phys_start = 0x00280000;

SECTIONS
{
    . = _kernel_phys_start;

    _kernel_virt_start = .;       

    .text ALIGN(16) :
    {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4) :
    {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4096) :
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    _kernel_virt_end = .;          
    _kernel_phys_end = .;         

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
        *(.note*)
    }
}