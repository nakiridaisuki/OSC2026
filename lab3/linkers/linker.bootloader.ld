OUTPUT_ARCH( "riscv" )
ENTRY( _start ) /* Set the entry point */

SECTIONS
{
    /* Starting address */
    . = 0x20000000;
    _start_address = .;

    .text : { 
        *(.text.entry) /* put entry point to the beginning */
        *(.text)
    }

    .rodata : { 
        *(.rodata .rodata.*)
    }
    .data : { 
        *(.data .data.*)
    }

    .bss : { 
        . = ALIGN(8);
        _bss_start = .; /* get bss start for boot.S */
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .; /* get bss end for boot.S */
    }

    /* By standard of RISC-V, stack pointer must have 16-type align */
    . = ALIGN(16);
    _stack_bottom = .;

    /* Leave 128 KB for stack */
    . += 128 * 1024;

    . = ALIGN(16);
    _stack_top = .;

    _end_address = .;
}
