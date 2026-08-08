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

In `bootloader_entry.S`, we use `lla` to get the current start address in memory.
To get desire starting address we defined in the linker script, we need to store it as a variable in our assembly.

```asm
_link_start_ptr:
    .quad _start_address
_link_end_ptr:
    .quad _end_address
_relocation_done_ptr:
    .quad relocation_done
```

`.quad` will allocate an 8-byte area for data and the data will be filled by the linker.
This data will be read like this:

```asm
lla t4, _link_start_ptr
ld t1, 0(t4)
```

Why we can't use the method like `lla` to load the desired memory address is because when we use `lla`,
the compiler will expand it into `auipc` + `addi` to calculate relative distance from current position.
(so as we use `la` with `--mcmodel=medany` flag)

If we write something like this:

```asm
lla t1, _start_address
```

The relative distance between `_start_address` and `_start` symbol will always be 0.
So `t1` will be current position, not the desired position.

## Basic Exercise 2

To parse the flattened devicetree (FDT) files, I designed a state machine.
Pass an iterator into it, it will return the node/property's data.
My iterator structure looks like this:

```c
typedef struct {
    const uint8_t *cursor;
    const uint8_t *strings;
    int depth;

    const uint8_t *event_start;
    const char *name;
    const uint8_t *val;
    uint32_t len;
    uint32_t nameoff;
} FDTIterator;
```

This architecture let us use one `fdt_next` function handle complete FDT parsing, and return a structured data.
Then we can focus on what types of data we need in each functional function like `get_fdt_node` or `get_fdt_prop`.

The main target of this exercise is get the UART base address from device tree.
By the data from internet, the UART node path usually specified in property `stdout-path` under `/chosen` node.
In the .dts of our lab, it looks like this:

```text
chosen {
    bootargs = "earlycon=sbi console=ttyS0,115200n8 loglevel=8 swiotlb=65536 rdinit=/init";
    stdout-path = "serial0:115200n8";
    linux,initrd-start = <0x00000000 0x00000000>;
    linux,initrd-end   = <0x00000000 0x00000000>;
};
```

The value separate by the colon, the front half is UART node name or aliases, the back half is UART setup data.
If the node name doesn't start with `/`, that means this is a aliases, you should find the real path in `/aliases` node.

In the `/aliases` node, we can get the real UART node path:

```text
aliases {
    serial0 = "/soc/serial@d4017000";
    ...
}
```

## Basic Exercise 3

In this exercise, we need to implement a CPIO parser.
