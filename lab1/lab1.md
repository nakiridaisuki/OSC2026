# Lab 1

This section will use UART interface to implement a minimal interactive shell.

## Basic Exercise 1

During boot, the bootloader loads our kernel image to a designated physical address and begins execution from there.
In the RISC-V ecosystem, this address is `0x80200000`, so we set the starting address in our linker script to this value.

```asm
...
SECTIONS
{
    /* Starting address */
    . = 0x80200000;

    ...
}
```

Before executing our main kernel program, we need to initialize two things:

1. The `.bss` segment must be zero-initialized according the C standard.
2. The stack pointer must be set to valid memory location for correct function call.

Initialize them using the following code:

```asm boot.S
_start:
    // Get stack top address from linker script
    la sp, _stack_top 

    // Get bss start/end from linker script
    la t0, _bss_start
    la t1, _bss_end

// BSS clean loop
clear_bss_loop:
    bgeu t0, t1, bss_cleaned

    sd zero, 0(t0)
    addi t0, t0, 8

    j clear_bss_loop
```

There are two cool things in this code:

1. We get three symbols from the linker script.
2. `.bss` section should be 8-byte aligned.

The `_stack_top`, `_bss_start` and `_bss_end` are symbols, and we need to define them somewhere.
Since we can obtain these values from the linker script, we define them there.

```asm linker.ld
.bss : { 
    _bss_start = .; /* get bss start for boot.S */
    *(.bss .bss.*)
    _bss_end = .; /* get bss end for boot.S */
}

_stack_bottom = .;

/* Leave 128 KB for stack */
. += 128 * 1024;

_stack_top = .;
```

The `.` means the current memory address, just like the 'current directory' in a shell.
So we record the `bss` start address before combining all `bss` sections, and record the end address after combining.
The same applies to the stack.

Next, the `sd` instruction requires the address to be 8-byte aligned, so we need to tell the linker to do so.

```asm linker.ld
.bss : { 
    . = ALIGN(8);
    _bss_start = .; /* get bss start for boot.S */
    *(.bss .bss.*)
    . = ALIGN(8);
    _bss_end = .; /* get bss end for boot.S */
}
```

Finally, we have to make sure our `boot.S` is placed at the beginning of the `.text` section and serves as the entry point.
So we give `boot.S` a special section name and an entry point label.

```asm boot.S
// Global entry point
.section .text.entry
.global _start 
...
```

In the linker script, we can use this information to arrange our code.

```asm linker.ld
OUTPUT_ARCH( "riscv" )
ENTRY( _start ) /* Set the entry point */

SECTIONS
{
    ...

    .text : { 
        *(.text.entry) /* put entry point to the beginning */
        *(.text)
    }

    ...
}
```

You should check the final `boot.S` and `linker.ld` in this directory.

## Basic Exercise 2

After initialization, we can write a main function like what we usually do now.
In this exercise, we need to setup the UART.

Honestly, I don't really understand what did I do, following are just some record.

First, to control devices on this board, we just need to read and write some data in registers.
For example, it works like a mail box.
You put a char (a 1 byte data) in it, and UART will take it and send it to host by itself.

So, what we need to do is:

1. Find where is our UART address.
2. Check if it's mail box working.
3. Put/Take the data from its mail box.

### Find the address

In the [SoC User Manual](https://github.com/nycu-caslab/OSC2026/raw/main/references/K1_User_Manual_(V6.1_2025.08.06).pdf) chapter 6.2,
we can find the UART0's address is `0xD401_7000`.
