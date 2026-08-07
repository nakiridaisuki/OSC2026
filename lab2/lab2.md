# Lab 2

## Fix of Lab1

In `lab1.md` I said that the kernel starts from `0x80200000` on the board,
but it's actually `0x00200000` since the board's memory starts from zero.
So in the linker script, it should be fixed like this:

```text
SECTIONS
{
    /* Starting address */
    . = 0x00200000;
...
}
```

And the `0x80200000` address is for the QEMU emulator, so there is a separate linker script for QEMU in this directory.

## Makefile Architecture

In this lab, we will have three different compile results for the real/qemu kernel and the bootloader.
They have different source code and linker script, so I wrote three separate makefiles.
The main `Makefile` looks like this:

```makefile
all:
 make -f Makefile.bootloader
 make -f Makefile.kernel

bootloader:
 make -f Makefile.bootloader

kernel:
 make -f Makefile.kernel

qemu:
 make -f Makefile.qemu run

test:
 make -f Makefile.test

clean:
 rm -rf build
 rm *.fit

```

So you can build what you want using a command like `make <target>` or simply `make` for the kernel and bootloader.
The `test` is for function testing on the host, and the `qemu` will run automatically after compilation.

## Basic Exercise 1 / Advance Exercise

In first exercise, we need to write a bootloader which can load real kernel via UART.
The process of our bootloader is:

```text
U-boot 
-> our bootloader at 0x00200000 (load by uboot)
-> relocate to       0x02000000 (by bootloader_entry.S)
-> wait/receive kernel and write into 0x00200000
-> fence.i flash all data and instruction (really important!!)
-> jump to 0x00200000
```

In this process, we jump to new bootloader/kernel code two times (relocate and kernel).
Before jumping, it's important to use `fence.i` to make sure every instruction and data will be read from new code region.

Our transmit protocol is very simple.
Start with a magic number `0x544F4F42`, then a 32-bit integer for the file size, then the binary kernel file.
