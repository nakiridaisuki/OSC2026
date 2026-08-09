#include "cpio.h"
#include "malloc.h"
#include "printf.h"
#include "shell.h"
#include "uart.h"
#include <stdint.h>

int main(unsigned long hartid, const uint8_t *fdt_ptr) {

    uart_init_from_fdt(fdt_ptr);
    printf("UART Initialize successfully!\n");

    cpionewc_init_from_fdt(fdt_ptr);
    printf("initrd start address: 0x%p\n", CPIO_START_ADDR);

    init_palloc();
    uint8_t *mem = palloc(1024);
    printf("Allocated memory at 0x%p\n", mem);

    uint8_t *mem1 = palloc(8192);
    printf("Allocated memory at 0x%p\n", mem1);

    uint8_t *mem2 = palloc(8192);
    printf("Allocated memory at 0x%p\n", mem2);

    pfree(mem);
    pfree(mem1);
    pfree(mem2);

    shell();

    return 0;
}
