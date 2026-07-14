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

## Basic Exercise 2/3

After initialization, we can write a main function like what we usually do now.
In this exercise, we need to setup the UART.

Honestly, I don't really understand what did I do, following are just some record.

First, to control devices on this board, we just need to read and write some data in registers.
For example, it works like a mail box.
You put a char (a 1 byte data) in it, and UART will take it and send it to host by itself.

So, what we need to do is:

1. Find where is our UART address.
2. Initialize the mail box.
3. Put/Get the data from its mail box.

### Find the UART base address

In the [SoC User Manual](https://github.com/nycu-caslab/OSC2026/raw/main/references/K1_User_Manual_(V6.1_2025.08.06).pdf) chapter 6.2,
we can find the UART0's address is `0xD401_7000`.

### Initialize the UART

The UART detail registers description is in chapter 16.3.

The UART unit on this chip has DMA function and a 64 bytes FIFO buffer.
For simplicity, we neightor use DMA nor FIFO and use polling instead of interrupt.

First, in chapter 16.3.4.5, Interrupt Enable Register(IER),
we can find the UUE(UART Unit Enable) and DMAE(DMA Requests Enable) field.
We only need to set the UUE to 1, so `IER = 0x40`.

```c
// Disable all interrupt and enable UART unit
write_reg(UART_IER, 0x40); // UUE at bit 6
```

Next, in chapter 16.3.4.7, FIFO Control Register(FCR),
we don't use FIFO buffer, so we clean and disable it.
The reset transmit and receive FIFO field at bit 1 and 2, so we first set `FCR = 0x06`.
Then set `FCR = 0x00` to disable the FIFO function.

```c
// Disable FIFO, clean FIFO first
write_reg(UART_FCR, 0x06); // reset transmit/receive FIFO at bit 1, 2
write_reg(UART_FCR, 0x00); // disable FIFO
```

Finally, we need to set baud rate divisor to make sure the UART has correct baud rate.
We first calculate the low and high 8 bits divisor.

```c
unsigned int divisor = (UART_CLK + (baudrate * 8)) / (baudrate * 16);
unsigned char dll    = divisor & 0xff;
unsigned char dlh    = (divisor >> 8) & 0xff;
```

In chapter 16.3.4.8, Line Control Register(LCR),
we need to enable divisor latch access to set the divisor, which at bit 7.
Then set the low and high divisor register.
Then set `LCR = 0x03` to get 8 data bit, no parity, 1 stop bit (8N1) serial-data format.

```c
// Setting Baud Rate
write_reg(UART_LCR, read_reg(UART_LCR) | 0x80);
write_reg(UART_DLL, dll);
write_reg(UART_DLH, dlh);
// Setting 8N1 data format
write_reg(UART_LCR, 0x03);
```

### Put/Get char from the UART

To implement a polling put/get char function, we only need to check the status of mail box using a while loop.
In chapter 16.3.4.10, Line Status Register(LSR),
the field `TDRQ` and `DR` tells us if the send and receive mail box is ready respectively.
If the send mail box is ready, we can write a data into it.
If the receive mail box is ready, we can read data from it.

To ensure proper display across different serial terminal, I replace the character `\n` into `\r\n`.

```c
void uart_putchar(char c) {
    while ((read_reg(UART_LSR) & LSR_TDRQ) == 0) {
    }

    if (c == '\n') {
        write_reg(UART_THR, (unsigned int)'\r');
        while ((read_reg(UART_LSR) & LSR_TDRQ) == 0) {
        }
    }

    write_reg(UART_THR, (unsigned int)c);
}

char uart_getchar() {
    while ((read_reg(UART_LSR) & LSR_DR) == 0) {
    }

    return read_reg(UART_RBR) & 0xff;
}
```

Now, we can I/O our orangepi with UART now.
To implement a simple shell, I use the printf library from [here](https://github.com/mpaland/printf) for output formatting.
And implement a simple `string.h` contain `strcmp` and `memset` to compare and initialize string.

The shell code is in `main.c`, check it if needed.

## Basic Exercise 4

In this exercise, we need to implement a SBI(Supervisor Binary Interface) call wrapper and use it to quire system information from OpenSBI.

Our kernel will run under supervisor-mode.
The SBI allows supervisor-mode software to be portable across all RISC-V chips.
It gives an abstraction of very low-level hardware control.

For example, to power off the system,
chip A needs to write `0x555` into register `0x100`,
chip B needs to communicate with power management IC,
chip C needs to low a GPIO pin, etc....
The SBI gives a `sbi_system_reset()` for this function so that our kernel don't need to handle hundred of conditions.

The SBI has several extensions, each extension has some functions.
All SBI functions share a single binary encoding:

- An `ecall` is used to call a SBI instruction.
- `a7` encodes the SBI extension ID(EID).
- `a6` encodes the SBI function ID(FID) in a SBI extension.
- `a0` ~ `a5` are function arguments.
- SBI functions return error code in `a0` and result value in `a1`.

For other details and SBI EDI and FID, check the [RISC-V SBI Reference](https://raw.githubusercontent.com/nycu-caslab/OSC2026/main/references/riscv-sbi.pdf).

If we want to write a wrapper of SBI call in C, we must have the ability to:

1. Write a assembly in C code.
2. Force a variable in expected register.

### 1. Inline Assembly

To write a assembly, we can use the following format:

```c
asm volatile (
    "<assembly code>"
    : <output operands>
    : <input operands>
    : <clobber list>
)
```

The `asm` is the inline assembly key word.
The `volatile` key word prevent this assembly optimized by compiler.

For example, our wrapper looks like this:

```c
asm volatile(
    "ecall"
    : "+r"(a0), "+r"(a1)
    : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
    : "memory"
);
```

The `ecall` is our SBI instruction.
The `r` means require a general register for this variable.
The `+` before `r` means this register is both readable and writable.
The `memory` clobber list tells the compiler this instruction may read-write any memory in system.
So all variables in register need to be flushed into memory before execution,
and re-read after execution. Also, it prevent instruction rearrange.

With this settings, our SBI call can read latest data and won't be rearranged.

### 2. Force Register

By the definition of the SBI, input and output need to at register `a0`~`a7`.
The inline assembly only assign a random register to us, we need to use following command to force the register:

```c
register unsigned long a0 asm("a0") = arg0;
```

Check `sbi.c` in `src` if needed.
