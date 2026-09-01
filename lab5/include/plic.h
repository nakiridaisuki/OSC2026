#ifndef _PLIC_H_
#define _PLIC_H_

#include <stdint.h>

#define MAX_IRQS 64

typedef void (*irq_handler_t)(uint32_t irq, void *context);

void init_plic(const uint8_t *fdt_ptr);
void plic_register(uint32_t irq, irq_handler_t handler, void *context);
void plic_enable(uint32_t irq, uint8_t priority);

#endif // !_PLIC_H_
