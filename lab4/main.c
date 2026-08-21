#include "cpio.h"
#include "dtb.h"
#include "malloc.h"
#include "plic.h"
#include "printf.h"
#include "sbi.h"
#include "shell.h"
#include "timer.h"
#include "trap.h"
#include "uart.h"
#include <stdbool.h>
#include <stdint.h>

static char user_stack[4096];

void fake_user(void) {
    printf("Hello, I'm user.\n");
    printf("ecall now\n");
    asm volatile("ecall");
    printf("Back\n");
    while (1) {
    }
}

void exec(void (*func)(void)) {
    asm volatile("csrw sepc, %0" : : "r"((uint64_t)func));
    printf("Set user function address.\n");

    uint64_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    sstatus &= ~(1UL << 8); // SPP = 0 enter U-mode after sret
    sstatus |= (1UL << 5);  // SPIE = 1 enable U-mode interrupt
    asm volatile("csrw sstatus, %0" : : "r"(sstatus));
    printf("Enable U-mode interrupt.\n");

    uint64_t user_sp = (uint64_t)&user_stack[4096];
    printf("Set user stack.\n");

    asm volatile("mv sp, %0\n"
                 "sret\n"
                 :
                 : "r"(user_sp));
}

int main(unsigned long hartid, const uint8_t *fdt_ptr) {
    init_trap();
    printf("Trap initialized.\n");

    init_plic(fdt_ptr);
    printf("PLIC initialized.\n");

    init_uart(fdt_ptr, true);
    printf("UART Initialized.\n");

    cpionewc_init_from_fdt(fdt_ptr);
    printf("initrd start address: 0x%p\n", CPIO_START_ADDR);

    init_malloc(fdt_ptr);
    printf("Malloc initialized\n");

    init_timer(fdt_ptr);
    printf("Timer initialized\n");

    // exec(fake_user);

    shell();

    return 0;
}
