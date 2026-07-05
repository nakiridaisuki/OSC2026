TARGET   ?= kernel
OBJS     ?= main.o
DTB_NAME := x1_orangepi-rv2.dtb

PREFIX  ?= riscv64-unknown-elf-
CC      := $(PREFIX)gcc
LD      := $(PREFIX)ld
OBJCOPY := $(PREFIX)objcopy
MKIMAGE := mkimage

COMMON_DIR := ../common
DTB        := $(COMMON_DIR)/$(DTB_NAME)
ITS        := $(COMMON_DIR)/kernel.its

LDFLAGS := -T linker.ld

all: $(TARGET).fit

%.o: %.s
	$(CC) -c $< -o $@

%.o: %.S
	$(CC) -c $< -o $@

%.o: %.c
	$(CC) -c $< -o $@

$(TARGET).elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

kernel.bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).fit: kernel.bin
	cp $(DTB) .
	cp $(ITS) .
	$(MKIMAGE) -f kernel.its $@
	rm -f $(DTB_NAME) kernel.its

clean:
	rm -f *.o *.elf *.bin *.fit $(DTB_NAME) kernel.its

run: kernel.bin
	qemu-system-riscv64 -M virt -kernel kernel.bin -nographic
