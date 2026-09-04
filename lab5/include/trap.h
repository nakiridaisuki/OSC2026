#ifndef _TRAP_H_
#define _TRAP_H_

#include "types.h"
#include <stdint.h>

typedef struct {
    union {
        uint64_t regs[32];
        struct {
            uint64_t zero; // x0
            uint64_t ra;   // x1  (Return Address)
            uint64_t sp;   // x2  (Stack Pointer)
            uint64_t gp;   // x3  (Global Pointer)
            uint64_t tp;   // x4  (Thread Pointer)
            uint64_t t0;   // x5
            uint64_t t1;   // x6
            uint64_t t2;   // x7
            uint64_t s0;   // x8  (Saved Register / Frame Pointer)
            uint64_t s1;   // x9
            uint64_t a0;   // x10 (Function Argument / Return Value)
            uint64_t a1;   // x11
            uint64_t a2;   // x12
            uint64_t a3;   // x13
            uint64_t a4;   // x14
            uint64_t a5;   // x15
            uint64_t a6;   // x16
            uint64_t a7;   // x17
            uint64_t s2;   // x18
            uint64_t s3;   // x19
            uint64_t s4;   // x20
            uint64_t s5;   // x21
            uint64_t s6;   // x22
            uint64_t s7;   // x23
            uint64_t s8;   // x24
            uint64_t s9;   // x25
            uint64_t s10;  // x26
            uint64_t s11;  // x27
            uint64_t t3;   // x28
            uint64_t t4;   // x29
            uint64_t t5;   // x30
            uint64_t t6;   // x31
        };
    };
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

static inline void _cleanup_intr(int *flag) { intr_restore(*flag); }

#define ATOMIC                                                                                   \
    for (int _flag __attribute__((cleanup(_cleanup_intr))) = intr_save_and_disable(), _done = 0; \
         !_done;                                                                                 \
         _done = 1)

void init_trap();
void trap_handler(TrapFrame *tf);
void register_local_intr(uint32_t code, intr_handler_t handler);
void register_exception(uint32_t code, excep_handler_t handler);
void trap_add_task(callback_t callback, void *args, int priority);

#endif // !_TRAP_H_
