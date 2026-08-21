#ifndef _TRAP_H_
#define _TRAP_H_

#include "cpio.h"
#include <stdint.h>

typedef struct {
    uint64_t regs[32];
    uint64_t sstatus;
    uint64_t sepc;
} TrapFrame;
typedef void (*trap_handler_t)(uintptr_t sepc, uintptr_t stval, void *context);

extern void trap_entry(void);

static inline uint64_t intr_save_and_disable(void) {
    uint64_t sstatus;
    asm volatile("csrrci %0, sstatus, 2" : "=r"(sstatus));
    return sstatus & 2;
}

static inline void intr_restore(uint64_t prev_sie) {
    if (prev_sie)
        asm volatile("csrs sstatus, 2");
}

void init_trap();
void trap_handler(TrapFrame *tf);
void register_local_intr(uint32_t code, trap_handler_t handler);
void register_exception(uint32_t code, trap_handler_t handler);

#endif // !_TRAP_H_
