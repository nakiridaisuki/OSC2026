#include "thread.h"
#include "dstruc.h"
#include "malloc.h"
#include "printf.h"
#include "string.h"
#include "trap.h"
#include <stdint.h>

extern int switch_to(ThreadCtx *prev, ThreadCtx *next);
extern int trap_restore();

static long global_tid = 1;

static ThreadCtx _idle_thd;
static ThreadCtx *curr_thd = &_idle_thd;
static uint8_t _idle_stack[128];

static LinkedListNode *idle_list = &_idle_thd.list;
static LinkedListNode wait_list;
static LinkedListNode zonbies_list;

void kill_zombies() {
    while (!lln_empty(&zonbies_list)) {
        LinkedListNode *tmp_n = lln_pop_front(&zonbies_list);
        ThreadCtx *tmp_ctx    = container_of(tmp_n, ThreadCtx, list);
        free(tmp_ctx->k_stack);
        if (tmp_ctx->u_stack)
            free(tmp_ctx->u_stack);
        free(tmp_ctx);
    }
}
void idle() {
    while (1) {
        kill_zombies();
        schedule();
    }
}

void init_thread() {
    _idle_thd.ra = (uintptr_t)idle;
    _idle_thd.sp = (uintptr_t)_idle_stack + sizeof(_idle_stack);
    memset(_idle_thd.sx, 0, sizeof(_idle_thd.sx));
    _idle_thd.k_stack = _idle_stack;
    _idle_thd.u_stack = NULL;
    lln_init(&_idle_thd.list);

    lln_init(&wait_list);
    lln_init(&zonbies_list);
}

void thread_create(void (*func)(void)) {
    ThreadCtx *ctx = (ThreadCtx *)malloc(sizeof(ThreadCtx));
    void *th_stack = malloc(4096);

    ctx->ra = (uintptr_t)func;
    ctx->sp = (uintptr_t)th_stack + 4096;
    memset(ctx->sx, 0, sizeof(ctx->sx));
    ctx->k_stack = th_stack;
    ctx->u_stack = NULL;
    ctx->tid     = global_tid++;
    lln_init(&ctx->list);

    int flag = intr_save_and_disable();
    lln_push_back(idle_list, &ctx->list);
    intr_restore(flag);
}
long thread_fork(TrapFrame *tf) {
    ThreadCtx *new_ctx = (ThreadCtx *)malloc(sizeof(ThreadCtx));

    // Handle kernel stack
    // put trap frame to the kernel stack of new thread
    void *k_stack = malloc(4096);
    new_ctx->ra   = (uintptr_t)trap_restore; // TODO fix it
    new_ctx->sp   = (uint64_t)k_stack + 4096 - sizeof(TrapFrame);
    memcpy((char *)new_ctx->sp, tf, sizeof(TrapFrame));
    TrapFrame *new_tf = (TrapFrame *)new_ctx->sp;

    // Handle user stack
    // copy full user stack and update necessery value in trap frame of new thread
    char *u_stack      = (char *)malloc(4096);
    size_t u_stack_len = (char *)curr_thd->u_stack + 4096 - (char *)tf->sp;
    new_tf->sp         = (uint64_t)(u_stack + 4096 - u_stack_len);
    memcpy((char *)new_tf->sp, (char *)tf->sp, u_stack_len);
    new_tf->a0 = 0;
    new_tf->sepc += 4;

    new_ctx->k_stack = k_stack;
    new_ctx->u_stack = u_stack;
    new_ctx->tid     = global_tid++;
    lln_init(&new_ctx->list);

    int flag = intr_save_and_disable();
    lln_push_back(idle_list, &new_ctx->list);
    intr_restore(flag);
    return new_ctx->tid;
}

void thread_exit() {
    ThreadCtx *ctx = get_current();
    lln_push_back(&zonbies_list, &ctx->list);
    switch_to(NULL, &_idle_thd);
}

void schedule() {
    if (lln_empty(idle_list))
        return;

    int flag = intr_save_and_disable();

    LinkedListNode *next_node = lln_pop_front(idle_list);
    ThreadCtx *next_thd       = container_of(next_node, ThreadCtx, list);

    if (curr_thd != &_idle_thd) {
        lln_push_back(idle_list, &curr_thd->list);
    }
    ThreadCtx *tmp_thd = curr_thd;
    curr_thd           = next_thd;

    switch_to(tmp_thd, next_thd);

    intr_restore(flag);
}

ThreadCtx *get_current() { return curr_thd; }
