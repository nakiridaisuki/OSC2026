#ifndef _TRAP_H_
#define _TRAP_H_

#include "cpio.h"
#include <stdint.h>

extern phys_addr_t PLIC_BASE;

#define PLIC_PRIO_BASE   PLIC_BASE
#define PLIC_ENABLE_BASE (PLIC_BASE + 0x2000)
#define PLIC_CTX_BASE    (PLIC_BASE + 0x200000)

#define PLIC_SET_PRIO(irq_id, val) \
    (*(volatile uint32_t *)(PLIC_PRIO_BASE + irq_id * 4) = (uint32_t)val)
#define PLIC_ENABLE(ctx_id, irq_id)                                            \
    (((volatile uint32_t *)(PLIC_ENABLE_BASE + ctx_id * 0x80))[irq_id / 32] |= \
     (1U << (irq_id % 32)))
#define PLIC_SET_PRIO_THLD(ctx_id, val) \
    (*(volatile uint32_t *)(PLIC_CTX_BASE + ctx_id * 0x1000) = (uint32_t)val)
#define PLIC_CLAME(ctx_id) (*(volatile uint32_t *)(PLIC_CTX_BASE + ctx_id * 0x1000 + 4))
#define PLIC_COMPLETE(ctx_id, irq) \
    (*(volatile uint32_t *)(PLIC_CTX_BASE + ctx_id * 0x1000 + 4) = (irq))

typedef struct {
    uint64_t regs[32];
    uint64_t sstatus;
    uint64_t sepc;
} TrapFrame;

extern void trap_entry(void);

void init_trap(const uint8_t *fdt_ptr);
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
