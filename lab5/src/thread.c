#include "thread.h"
#include "dstruc.h"
#include "malloc.h"
#include "printf.h"
#include "string.h"
#include <stdint.h>

extern int switch_to(ThreadCtx *prev, ThreadCtx *next);

static int global_tid = 0;

static ThreadCtx _idle_thd;
static ThreadCtx *curr_thd = &_idle_thd;
static uint8_t _idle_stack[128];

static LinkedListNode *idle_list = &_idle_thd.list;
static LinkedListNode wait_list;
static LinkedListNode zonbies_list;

void idle() {
    while (1) {
        kill_zombies();
        schedule();
    }
}

void init_thread() {
    _idle_thd.ra = (uintptr_t)idle;
    _idle_thd.sp = (uintptr_t)_idle_stack;
    memset(_idle_thd.sx, 0, sizeof(_idle_thd.sx));
    _idle_thd.stack = _idle_stack + sizeof(_idle_stack);
    lln_init(&_idle_thd.list);

    lln_init(&wait_list);
    lln_init(&zonbies_list);
}

void thread_create(void (*func)(void)) {
    ThreadCtx *ctx = (ThreadCtx *)malloc(sizeof(ThreadCtx));
    void *th_stack = malloc(4096);

    ctx->ra = (uintptr_t)func;
    ctx->sp = (uintptr_t)th_stack;
    memset(ctx->sx, 0, sizeof(ctx->sx));
    ctx->stack = th_stack + 4096;
    ctx->tid   = global_tid++;
    lln_init(&ctx->list);

    lln_push_back(idle_list, &ctx->list);
}

void thread_exit() {
    ThreadCtx *ctx = get_current();
    lln_push_back(&zonbies_list, &ctx->list);
    switch_to(NULL, &_idle_thd);
}
void kill_zombies() {
    while (!lln_empty(&zonbies_list)) {
        LinkedListNode *tmp_n = lln_pop_front(&zonbies_list);
        ThreadCtx *tmp_ctx    = container_of(tmp_n, ThreadCtx, list);
        free(tmp_ctx->stack);
        free(tmp_ctx);
    }
}

void schedule() {
    if (lln_empty(idle_list))
        return;

    LinkedListNode *next_node = lln_pop_front(idle_list);
    ThreadCtx *next_thd       = container_of(next_node, ThreadCtx, list);

    if (curr_thd != &_idle_thd) {
        lln_push_back(idle_list, &curr_thd->list);
    }
    ThreadCtx *tmp_thd = curr_thd;
    curr_thd           = next_thd;

    switch_to(tmp_thd, next_thd);
}

ThreadCtx *get_current() { return curr_thd; }
