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
    sprintf(buf, "Hi, I am user thread %ld, list %p\n", getpid(), get_current());
    uart_write(buf, strlen(buf));

    int cid;
    if ((cid = fork()) != 0) {
        sprintf(buf, "Child pid %ld\n", cid);
        uart_write(buf, strlen(buf));

        int ccid;
        if ((ccid = fork()) != 0) {
            sprintf(buf, "wait child %ld\n", cid);
            uart_write(buf, strlen(buf));
            waitpid(cid);
        } else {
            sprintf(buf, "stop child %ld\n", cid);
            uart_write(buf, strlen(buf));
            stop(cid);
        }
    } else {
        while (1) {
            printf("HI");
        }
    }
    sprintf(buf, "Thread %ld exit\n", getpid());
    uart_write(buf, strlen(buf));

    exit(0);
}

void fork_test() {
    printf("Fork test (pid = %d)\n", getpid());
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) {
        long cur_sp;
        asm volatile("mv %0, sp" : "=r"(cur_sp));
        printf("child1: pid = %d, cnt = %d, &cnt = %p, sp = %p\n", getpid(), cnt, &cnt, cur_sp);
        cnt++;

        if ((ret = fork()) != 0) {
            asm volatile("mv %0, sp" : "=r"(cur_sp));
            printf("child1: pid = %d, cnt = %d, &cnt = %p, sp = %p\n", getpid(), cnt, &cnt, cur_sp);
            waitpid(ret);
        } else {
            while (cnt < 5) {
                asm volatile("mv %0, sp" : "=r"(cur_sp));
                printf(
                    "child2: pid = %d, cnt = %d, &cnt = %p, sp = %p\n", getpid(), cnt, &cnt, cur_sp
                );
                for (int i = 0; i < 1000000000; i++)
                    ;
                cnt++;
            }
        }
    } else {
        printf("parent: pid = %d, child pid = %d\n", getpid(), ret);
        waitpid(ret);
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

void test() { _exec(fork_test); }

void init() {
    printf("Init program.\n");
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
                 : "r"(sstatus), "r"(shell), "r"(kernel_sp), "r"(user_sp + 4096)
                 : "memory");
}

extern void video_init();
extern void test_screen_color();

int main(unsigned long hartid, const uint8_t *fdt_ptr) {
    init_cpionewc(fdt_ptr);
    printf("initrd start address: 0x%lx\n", CPIO_START_ADDR);

    init_malloc(fdt_ptr);
    printf("Malloc initialized\n");

    init_trap();
    printf("Trap initialized.\n");

    init_plic(fdt_ptr);
    printf("PLIC initialized.\n");

    init_uart(fdt_ptr, true);
    printf("UART Initialized.\n");

    init_timer(fdt_ptr);
    printf("Timer initialized\n");

    init_thread();
    printf("Thread initialized\n");

    init_syscall();
    printf("System Call initialized\n");

    video_init();
    printf("Video initialized\n");

    // thread_create(test);
    thread_create(init);
    idle();
    // shell();

    return 0;
}
