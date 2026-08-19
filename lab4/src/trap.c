#include "trap.h"
#include "printf.h"
#include "sbi.h"
#include <stdint.h>

void init_trap() {
    asm volatile("csrw stvec, %0" : : "r"((uint64_t)trap_entry));
    asm volatile("csrw sscratch, zero");

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

    if (scause == (1ULL << 63 | 5)) {
        const uint64_t CPU_CLOCK = 0x16e3600;
        uint64_t curr_time;
        asm volatile("rdtime %0" : "=r"(curr_time));
        printf("Boot time %lu\n", curr_time / CPU_CLOCK);
        sbi_set_timer(curr_time + CPU_CLOCK * 2);
    }
}
