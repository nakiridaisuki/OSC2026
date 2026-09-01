#include "cpio.h"
#include "dtb.h"
#include "malloc.h"
#include "plic.h"
#include "printf.h"
#include "sbi.h"
#include "shell.h"
#include "thread.h"
#include "timer.h"
#include "trap.h"
#include "uart.h"
#include <stdbool.h>
#include <stdint.h>

void foo() {
    for (int i = 0; i < 5; i++) {
        printf("Thread id: %d print %d\n", get_current()->tid, i);
        for (int j = 0; j < 1000000; j++)
            ;
        schedule();
    }
    thread_exit();
}

void test_thread() {
    for (int i = 0; i < 3; i++) {
        thread_create(foo);
    }
    idle();
}

int main(unsigned long hartid, const uint8_t *fdt_ptr) {
    init_malloc(fdt_ptr);
    printf("Malloc initialized\n");

    init_trap();
    printf("Trap initialized.\n");

    init_plic(fdt_ptr);
    printf("PLIC initialized.\n");

    cpionewc_init_from_fdt(fdt_ptr);
    printf("initrd start address: 0x%lx\n", CPIO_START_ADDR);

    init_uart(fdt_ptr, true);
    printf("UART Initialized.\n");

    init_timer(fdt_ptr);
    printf("Timer initialized\n");

    init_thread();
    printf("Thread initialized\n");

    test_thread();

    shell();

    return 0;
}
