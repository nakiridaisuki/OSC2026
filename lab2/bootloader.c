#include "printf.h"
#include "uart.h"
#include <stdint.h>

#define MAGIC_NUM   0x544F4F42
#define KERNEL_BASE 0x00200000

void waiting_magic_num() {
    int state = 0;

    while (1) {
        char c = uart_getchar();
        if (state == 0 && c == (MAGIC_NUM & 0xFF)) {
            state = 1;
        } else if (state == 1 && c == (MAGIC_NUM >> 8 & 0xFF)) {
            state = 2;
        } else if (state == 2 && c == (MAGIC_NUM >> 16 & 0xFF)) {
            state = 3;
        } else if (state == 3 && c == (MAGIC_NUM >> 24 & 0xFF)) {
            break;
        } else {
            if (c == (MAGIC_NUM & 0xFF))
                state = 1;
            else
                state = 0;
        }
    }
}

void main(unsigned long hartid, const uint8_t *fdt_ptr) {
    uart_init_from_fdt(fdt_ptr);

    unsigned long current_pc;
    asm volatile("auipc %0, 0" : "=r"(current_pc));

    printf("\n=========================================\n");
    printf("Bootloader initialized successfully!\n");
    printf("Current Program Counter (PC) is: 0x%016lx\n", current_pc);
    printf("=========================================\n\n");

    printf("Waiting for connection...\n");

    waiting_magic_num();
    unsigned int kernel_size = uart_getuint32();
    unsigned char *load_addr = (unsigned char *)KERNEL_BASE;
    for (int i = 0; i < kernel_size; i++) {
        load_addr[i] = uart_getchar();
    }
    asm volatile("fence.i");

    printf("Connected! Kernel size is: %u bytes\n", kernel_size);
    printf("Kernel loaded at %p\n", load_addr);
    printf("Jump to kernel...\n");

    for (volatile int d = 0; d < 5000000; d++)
        ;

    void (*kernel_entry)(unsigned long, const uint8_t *) =
        (void (*)(unsigned long, const uint8_t *))load_addr;
    kernel_entry(hartid, fdt_ptr);
}
