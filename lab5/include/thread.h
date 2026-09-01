#ifndef _THREAD_H_
#define _THREAD_H_

#include "dstruc.h"
#include <stdint.h>

typedef struct {
    uintptr_t ra;     // return address
    uintptr_t sp;     // stack pointer
    uintptr_t sx[12]; // save pointers
    void *stack;      // thread stack base address
    LinkedListNode list;
    int tid;
} ThreadCtx;

void idle();
void init_thread();
void thread_create(void (*func)(void));
void thread_exit();
void kill_zombies();
void schedule();

ThreadCtx *get_current();
#endif // !_THREAD_H_
