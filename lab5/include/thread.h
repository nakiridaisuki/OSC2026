#ifndef _THREAD_H_
#define _THREAD_H_

#include "dstruc.h"
#include "trap.h"
#include <stdint.h>

typedef struct {
    uintptr_t ra;     // return address
    uintptr_t sp;     // stack pointer
    uintptr_t sx[12]; // save pointers
    void *k_stack;    // kernel stack base address
    void *u_stack;    // user stack base address
    LinkedListNode list;
    uint64_t tid;
} ThreadCtx;
typedef void (*func_t)(void *);

void idle();
void init_thread();
void thread_create(void (*func)(void));
long thread_fork(TrapFrame *tf);
void thread_exit();
void schedule();

ThreadCtx *get_current();
#endif // !_THREAD_H_
