OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(entry_start)

_kernel_phys_start = 0x00280000;
_kernel_virt_base  = 0xC0000000;

SECTIONS
{
    . = _kernel_virt_base + _kernel_phys_start;

    _kernel_virt_start = .;

    .text ALIGN(16) : AT(ADDR(.text) - _kernel_virt_base)
    {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4) : AT(ADDR(.rodata) - _kernel_virt_base)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4) : AT(ADDR(.data) - _kernel_virt_base)
    {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4096) : AT(ADDR(.bss) - _kernel_virt_base)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    _kernel_virt_end = .;
    _kernel_phys_end = . - _kernel_virt_base;

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
        *(.note*)
    }
}
