#ifndef _TRAP_H_
#define _TRAP_H_

#include "types.h"
#include <stdint.h>

typedef struct {
    uint64_t regs[32];
    uint64_t sstatus;
    uint64_t sepc;
} TrapFrame;
typedef void (*intr_handler_t)(void *context);
typedef void (*excep_handler_t)(TrapFrame *tf, uint64_t stval);

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
void register_local_intr(uint32_t code, intr_handler_t handler);
void register_exception(uint32_t code, excep_handler_t handler);
void trap_add_task(callback_t callback, void *args, int priority);

#endif // !_TRAP_H_
