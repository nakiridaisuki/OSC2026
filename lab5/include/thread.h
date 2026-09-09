#ifndef _THREAD_H_
#define _THREAD_H_

#include "dstruc.h"
#include "trap.h"
#include <stdint.h>

typedef struct {
    uintptr_t ra;     // return address
    uintptr_t sp;     // stack pointer
    uintptr_t sx[12]; // save pointers
    uint64_t tid;
    void *k_stack; // kernel stack base address
    void *u_stack; // user stack base address
    LinkedListNode list, wait_queue;
} ThreadCtx;
typedef void (*func_t)(void *);

void idle();
void init_thread();
void thread_create(void (*func)(void));
long thread_fork(TrapFrame *tf);
int thread_stop(long pid);
long thread_wait(long pid);
void thread_exit();
void thread_schedule();

ThreadCtx *get_current();
#endif // !_THREAD_H_
