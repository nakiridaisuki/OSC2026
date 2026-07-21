TARGET   ?= kernel
OBJS     ?= main.o
LINKER   ?= linker.ld
DTB_NAME := x1_orangepi-rv2.dtb

PREFIX  ?= riscv64-unknown-elf-
CC      := $(PREFIX)gcc
LD      := $(PREFIX)ld
OBJCOPY := $(PREFIX)objcopy
MKIMAGE := mkimage

COMMON_DIR := ../common
OBJ_DIR    ?= ./build
DTB        := $(COMMON_DIR)/$(DTB_NAME)
ITS        := $(COMMON_DIR)/kernel.its

CFLAGS  := -Wall -mcmodel=medany -ffreestanding -nostdlib -Iinclude
LDFLAGS := -T $(LINKER)

vpath %.c . src
vpath %.S . src
vpath %.s . src

REAL_OBJS := $(addprefix $(OBJ_DIR)/, $(OBJS))

all: $(TARGET).fit

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.S | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/$(TARGET).elf: $(REAL_OBJS) $(LINKER) | $(OBJ_DIR)
	$(LD) $(LDFLAGS) -o $@ $(REAL_OBJS)

$(OBJ_DIR)/kernel.bin: $(OBJ_DIR)/$(TARGET).elf | $(OBJ_DIR)
	$(OBJCOPY) -O binary $< $@

$(TARGET).fit: $(OBJ_DIR)/kernel.bin
	$(MKIMAGE) -f $(ITS) -D "-i $(COMMON_DIR) -i $(OBJ_DIR)" $@

clean:
	rm -rf $(OBJ_DIR)
	rm *.fit

run: $(OBJ_DIR)/kernel.elf
	qemu-system-riscv64 -M virt -kernel $(OBJ_DIR)/kernel.elf -nographic
