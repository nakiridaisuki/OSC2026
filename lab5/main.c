#include "cpio.h"
#include "dtb.h"
#include "malloc.h"
#include "plic.h"
#include "printf.h"
#include "sbi.h"
#include "shell.h"
#include "syscall.h"
#include "thread.h"
#include "timer.h"
#include "trap.h"
#include "uart.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void user_test(void) {
    char buf[256];
    sprintf(buf, "Hi, I am user thread %ld\n", getpid());
    uart_write(buf, strlen(buf));
    sprintf(buf, "Fork now\n");
    uart_write(buf, strlen(buf));
    long child_pid = fork();
    if (child_pid == 0) {
        sprintf(buf, "child process with pid %ld\n", getpid());
        uart_write(buf, strlen(buf));
        schedule();
    } else {
        sprintf(buf, "parent process with pid %ld\n", getpid());
        uart_write(buf, strlen(buf));
        schedule();

        sprintf(buf, "parent stop child now.\n");
        uart_write(buf, strlen(buf));
        stop(child_pid);
    }
    exit(0);
}

void _exec(void (*func)(void)) {
    uint64_t user_sp       = (uint64_t)malloc(4096);
    get_current()->u_stack = (void *)user_sp;
    uint64_t kernel_sp;
    asm volatile("mv %0, sp" : "=r"(kernel_sp));

    uint64_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    sstatus &= ~(1UL << 8); // SPP = 0 enter U-mode after sret
    sstatus |= (1UL << 5);  // SPIE = 1 enable U-mode interrupt

    asm volatile("csrc sstatus, 2\n" // disable interrupt
                 "csrw sstatus, %0\n"
                 "csrw sepc, %1\n"
                 "csrw sscratch, %2\n"
                 "mv sp, %3\n"
                 "sret\n"
                 :
                 : "r"(sstatus), "r"(func), "r"(kernel_sp), "r"(user_sp + 4096)
                 : "memory");
}
void foo() {
    for (int i = 0; i < 5; i++) {
        printf("Thread id: %d print %d\n", get_current()->tid, i);
        for (int j = 0; j < 1000000; j++)
            ;
        thread_schedule();
    }
    thread_exit();
}

void test_thread() {
    _exec(user_test);
    // for (int i = 0; i < 3; i++) {
    //     thread_create(foo);
    // }
    // idle();
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

    init_syscall();
    printf("System Call initialized\n");

    // test_thread();
    // _exec(user_test);
    thread_create(test_thread);
    idle();

    shell();

    return 0;
}
