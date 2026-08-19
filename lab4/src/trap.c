#include "trap.h"
#include "dstruc.h"
#include "printf.h"
#include "sbi.h"
#include "timer.h"
#include <stdint.h>
#include <stdlib.h>

void init_trap() {
    // set trap handler enter point
    asm volatile("csrw stvec, %0" : : "r"((uint64_t)trap_entry));

    // init interrupt sp reg to 0
    asm volatile("csrw sscratch, zero");

    // enable supervisor global interrupt
    asm volatile("csrs sstatus, 2");

    // enable supervisor time int
    sbi_set_timer(-1);
    uint64_t sei_mask = (1 << 5);
    asm volatile("csrs sie, %0" ::"r"(sei_mask));
}

void trap_handler(TrapFrame *tf) {
    uint64_t scause, stval, sepc;
    asm volatile("csrr %0, scause" : "=r"(scause));
    asm volatile("csrr %0, stval" : "=r"(stval));
    sepc = tf->sepc;

    printf("\n[Kernel Trap Handler] Caught an exception!\n");
    printf("  scause: 0x%lx\n", scause);
    printf("  sepc:   0x%lx\n", sepc);
    printf("  stval:  0x%lx\n", stval);

    if (scause == 8)
        tf->sepc += 4;

    if (scause & (1ULL << 63)) { // interrupt
        scause ^= (1ULL << 63);

        if (scause == 5) { // timer interrupt
            uint64_t now = __rdtime();
            while (MIN_TIMER != &TIMER_LIST_HEAD && MIN_TIMER->expires <= now) {
                Timer *timer = MIN_TIMER;
                lln_remove(&timer->list);

                if (timer->callback)
                    timer->callback(timer->arg);

                free(timer);
            }
            sbi_set_timer(MIN_TIMER->expires);
        } else
            while (1) {
            }

    } else { // exception
        if (scause == 8)
            tf->sepc += 4;
        else
            while (1) {
            }
    }
}
