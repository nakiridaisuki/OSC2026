#include "cpio.h"
#include "include/sbi.h"
#include "malloc.h"
#include "printf.h"
#include "sbi.h"
#include "shell.h"
#include "trap.h"
#include "uart.h"
#include <stdint.h>

static char user_stack[4096];

void fake_user(void) {
    asm volatile("ecall");

    while (1) {
        // asm volatile("ecall");
    }
}

void exec(void (*func)(void)) {
    asm volatile("csrw sepc, %0" : : "r"((uint64_t)func));

    uint64_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    sstatus &= ~(1UL << 8); // SPP = 0 enter U-mode after sret
    sstatus |= (1UL << 5);  // SPIE = 1 enable U-mode interrupt
    asm volatile("csrw sstatus, %0" : : "r"(sstatus));

    uint64_t user_sp = (uint64_t)&user_stack[4096];

    asm volatile("mv sp, %0\n"
                 "sret\n"
                 :
                 : "r"(user_sp));
}

int main(unsigned long hartid, const uint8_t *fdt_ptr) {

    uart_init_from_fdt(fdt_ptr);
    printf("UART Initialize successfully!\n");

    cpionewc_init_from_fdt(fdt_ptr);
    printf("initrd start address: 0x%p\n", CPIO_START_ADDR);

    init_malloc(fdt_ptr);
    printf("Malloc initialized\n");

    init_trap();
    asm volatile("csrs sstatus, 2");
    printf("Trap initialized\n");

    // exec(fake_user);

    uint64_t curr_time;
    asm volatile("rdtime %0" : "=r"(curr_time));
    printf("Current time is %lu\n", curr_time);

    sbi_set_timer(curr_time + 0x16e3600);

    shell();

    return 0;
}
