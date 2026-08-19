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

#endif // !_TRAP_H_
