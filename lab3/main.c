#include "cpio.h"
#include "printf.h"
#include "shell.h"
#include "uart.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int main(unsigned long hartid, const uint8_t *fdt_ptr) {

    uart_init_from_fdt(fdt_ptr);
    printf("UART Initialize successfully!\n");

    cpionewc_init_from_fdt(fdt_ptr);
    printf("initrd start address: %p\n", CPIO_START_ADDR);

    shell();

    return 0;
}
