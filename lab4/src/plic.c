#include "plic.h"
#include "dtb.h"
#include "printf.h"
#include "trap.h"
#include <stddef.h>

static phys_addr_t PLIC_BASE;
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
    irq_handler_t handler;
    void *context;
} IrqAction;

static IrqAction irq_table[MAX_IRQS];

static void default_irq_handler(uint32_t irq, void *context) {
    printf("[ERROR] Unknow irq 0x%x\n", irq);
    while (1) {
    }
}

static void plic_intr_handler(uintptr_t sepc, uintptr_t stval, void *context) {
    while (1) {
        uint32_t irq = PLIC_CLAME(1);
        if (irq == 0)
            break;
        irq_table[irq].handler(irq, context);
        PLIC_COMPLETE(1, irq);
    }
}

void plic_register(uint32_t irq, irq_handler_t handler, void *context) {
    irq_table[irq].handler = handler;
    irq_table[irq].context = context;
}

void init_plic(const uint8_t *fdt_ptr) {
    FDTHeader fdt_header          = get_fdt_header(fdt_ptr);
    const uint8_t *dt_struct_ptr  = fdt_ptr + fdt_header.off_dt_struct;
    const uint8_t *dt_strings_ptr = fdt_ptr + fdt_header.off_dt_strings;
    FDTProp reg =
        fdt_find_prop_by_path(dt_struct_ptr, dt_strings_ptr, "/soc/interrupt-controller", "reg");
    PLIC_BASE = fdt_read_u64_save(reg, 0);

    for (size_t i = 0; i < MAX_IRQS; i++) {
        irq_table[i].handler = default_irq_handler;
        irq_table[i].context = NULL;
    }

    PLIC_SET_PRIO_THLD(1, 0);
    register_local_intr(9, plic_intr_handler);
}

void plic_enable(uint32_t irq, uint8_t priority) {
    PLIC_SET_PRIO(irq, priority);
    PLIC_ENABLE(1, irq);
}
