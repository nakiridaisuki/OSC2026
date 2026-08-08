# Lab 0

In this lab, we need to set up our cross-platform development environment.
Follow [OSC2026 Lab0](https://nycu-caslab.github.io/OSC2026/labs/lab0.html)

## Cross Compiler

Our Orange Pi RV2 uses a 64-bit RISC-V processor,
so we need to compile our code into RISC-V machine code.

Our development environment is Linux on x86 system, we need a cross compiler to do that.
To install the cross compiler for RISC-V bare metal developing, using the following command.

### Ubuntu/Debian

```shell
sudo apt update
sudo apt install gcc-riscv64-unknown-elf
```

### Arch

On Arch Linux, you can install the AUR package to get the tools with the exact same names as those on Ubuntu/Debian:
This command will install both 32 and 64-bit tool chain.

```shell
yay -S riscv-gnu-toolchain-bin
```

## QEMU

We use qemu as an emulator to test our code on host machine.

Using the following commands to install qemu.

### Ubuntu/Debian

In modern Debian/Ubuntu versions, the RISC-V targets are packaged separately.
For older distributions, you might need to install `qemu-system-misc` instead.

```shell
sudo apt update
# For newer versions (Debian 12+ / Ubuntu 22.04+):
sudo apt install qemu-system-riscv64

# For older versions:
# sudo apt install qemu-system-misc
```

### Arch Linux

On Arch Linux, the official repository provides the standalone package for RISC-V system emulation.

```shell
sudo pacman -S qemu-system-riscv
```

**\\\\ Ctrl+a + x to exit qemu //** \
**\\\\ Ctrl+a + x to exit qemu //** \
**\\\\ Ctrl+a + x to exit qemu //** \

## Compile Script

By the flow in course site, I write a Makefile in `common/common.mk`.

`make` will generate the final FIT image file.\
`make run` will use qemu to test the `kernel.bin`.
