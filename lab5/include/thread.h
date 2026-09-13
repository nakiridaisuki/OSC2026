#ifndef _THREAD_H_
#define _THREAD_H_

#include "dstruc.h"
#include "timer.h"
#include "trap.h"
#include <stdint.h>

typedef enum { AVAIL, KILLED } ThreadStat;

typedef struct {
    uintptr_t ra;     // return address
    uintptr_t sp;     // stack pointer
    uintptr_t sx[12]; // save pointers
    uint64_t tid;
    void *k_stack;  // kernel stack base address
    void *u_space;  // user space base address
    uint64_t u_len; // user space len
    LinkedListNode list, wait_queue;
    Timer timer;
    ThreadStat stat;
} ThreadCtx;
typedef void (*func_t)(void *);

void idle();
void init_thread();
void thread_create(void (*func)(void));
long thread_fork(TrapFrame *tf);
int thread_stop(long pid);
long thread_wait(long pid);
int thread_sleep(unsigned int usec);
void thread_exit();
void thread_schedule();

ThreadCtx *get_current();
#endif // !_THREAD_H_
