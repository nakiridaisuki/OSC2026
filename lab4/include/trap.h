#ifndef _TRAP_H_
#define _TRAP_H_

#include <stdint.h>

typedef struct {
    uint64_t regs[32];
    uint64_t sstatus;
    uint64_t sepc;
} TrapFrame;

extern void trap_entry(void);

void init_trap();
void trap_handler(TrapFrame *tf);

static inline uint64_t intr_save_and_disable(void) {
    uint64_t sstatus;
    asm volatile("csrrci %0, sstatus, 2" : "=r"(sstatus));
    return sstatus & 2;
}

static inline void intr_restore(uint64_t prev_sie) {
    if (prev_sie)
        asm volatile("csrs sstatus, 2");
}

#endif // !_TRAP_H_
