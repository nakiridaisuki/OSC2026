#include "trap.h"
#include "printf.h"
#include "sbi.h"
#include <stddef.h>
#include <stdint.h>

#define MAX_LOCAL_INTR 16
#define MAX_EXCEPTIONS 64

static trap_handler_t local_intr_table[MAX_LOCAL_INTR];
static trap_handler_t exception_table[MAX_EXCEPTIONS];

static void default_handler(uintptr_t sepc, uintptr_t stval, void *context) {
    uint64_t scause;
    asm volatile("csrr %0, scause" : "=r"(scause));
    printf(
        "[ERROR] Unhandled trap! scause: 0x%lx, sepc: 0x%lx, stval: 0x%lx\n", scause, sepc, stval
    );
    while (1) {
    }
}

void register_local_intr(uint32_t code, trap_handler_t handler) {
    if (code < MAX_LOCAL_INTR)
        local_intr_table[code] = handler;
}
void register_exception(uint32_t code, trap_handler_t handler) {
    if (code < MAX_EXCEPTIONS)
        exception_table[code] = handler;
}

void init_trap() {
    // set trap handler enter point
    asm volatile("csrw stvec, %0" : : "r"((uint64_t)trap_entry));

    // init interrupt sp reg to 0
    asm volatile("csrw sscratch, zero");

    // enable supervisor global intr (SIE)
    asm volatile("csrs sstatus, 2");

    // enable supervisor time intr (STIE)
    sbi_set_timer(-1);
    asm volatile("csrs sie, %0" ::"r"(1UL << 5));

    // enable supervisor external intr (SEIE)
    asm volatile("csrs sie, %0" ::"r"(1UL << 9));

    for (size_t i = 0; i < MAX_LOCAL_INTR; i++)
        local_intr_table[i] = default_handler;
    for (size_t i = 0; i < MAX_EXCEPTIONS; i++)
        exception_table[i] = default_handler;
}

void trap_handler(TrapFrame *tf) {
    uint64_t scause, stval, sepc;
    asm volatile("csrr %0, scause" : "=r"(scause));
    asm volatile("csrr %0, stval" : "=r"(stval));
    sepc = tf->sepc;

    // printf("\n[Kernel Trap Handler] Caught an exception!\n");
    // printf("  scause: 0x%lx\n", scause);
    // printf("  sepc:   0x%lx\n", sepc);
    // printf("  stval:  0x%lx\n", stval);

    if (scause & (1ULL << 63)) { // interrupt
        scause ^= (1ULL << 63);
        local_intr_table[scause](sepc, stval, NULL);
    } else { // exception
        exception_table[scause](sepc, stval, NULL);
    }
}
