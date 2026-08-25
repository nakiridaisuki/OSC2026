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

void test_cb(void *args) {
    int *data = (int *)args;
    int id    = data[0];
    int prio  = data[1];
    printf("Test callback id %d with priority %d\n", id, prio);
}

void test_timer(void *args) { printf("Trigger.\n"); }

void high_prio_task(void *args) {
    printf("    [High Task] This is a high priority task.\n");
    for (int i = 0; i < 100000; i++) {
        asm volatile("nop");
    }
    printf("    [High Task] End.\n");
}

void low_prio_task(void *args) {
    printf("[Low Task] This is a low priority task.\n");
    printf("[Low Task] Calling a high priority task...\n");

    trap_add_task(high_prio_task, NULL, 1);

    // trigger task handling using timer intr.
    Timer timer;
    add_timer(&timer, 0, test_timer, NULL);

    printf("[Low Task] End.\n");
}

void task_qeueu_test() {
    int test1[2] = {1, 0};
    int test2[2] = {2, 1};
    int test3[2] = {3, 1};
    int test4[2] = {4, 2};
    printf("Add test task from 4 to 1\n");
    trap_add_task(test_cb, test4, test4[1]);
    trap_add_task(test_cb, test3, test3[1]);
    trap_add_task(test_cb, test2, test2[1]);
    trap_add_task(test_cb, test1, test1[1]);

    // trigger task handling using timer intr.
    Timer timer;
    add_timer(&timer, 0, test_timer, NULL);

    printf("Test Preemption\n");
    trap_add_task(low_prio_task, NULL, 3);

    // trigger task handling using timer intr.
    add_timer(&timer, 0, test_timer, NULL);
}

int main(unsigned long hartid, const uint8_t *fdt_ptr) {
    init_trap();
    printf("Trap initialized.\n");

    init_plic(fdt_ptr);
    printf("PLIC initialized.\n");

    cpionewc_init_from_fdt(fdt_ptr);
    printf("initrd start address: 0x%lx\n", CPIO_START_ADDR);

    init_malloc(fdt_ptr);
    printf("Malloc initialized\n");

    init_uart(fdt_ptr, true);
    printf("UART Initialized.\n");

    init_timer(fdt_ptr);
    printf("Timer initialized\n");

    task_qeueu_test();
    // exec(fake_user);

    shell();

    return 0;
}
