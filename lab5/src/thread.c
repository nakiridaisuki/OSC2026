#include "thread.h"
#include "dstruc.h"
#include "malloc.h"
#include "printf.h"
#include "string.h"
#include "timer.h"
#include "trap.h"
#include <stdint.h>

extern int switch_to(ThreadCtx *prev, ThreadCtx *next);
extern int trap_restore();

static long global_tid = 1;

static ThreadCtx _idle_thd;
static ThreadCtx *curr_thd = &_idle_thd;
static uint8_t _idle_stack[128];

static LinkedListNode *idle_list = &_idle_thd.list;
static LinkedListNode zombies_list;

static Timer switch_timer;

static void clean_thread(ThreadCtx *thd) {
    free(thd->k_stack);
    if (thd->u_stack)
        free(thd->u_stack);
    free(thd);
}
static void kill_zombies() {
    while (!lln_empty(&zombies_list)) {
        LinkedListNode *tmp_n = lln_pop_front(&zombies_list);
        clean_thread(container_of(tmp_n, ThreadCtx, list));
    }
}

static void _thd_timer_cb(void *args) {
    // printf("Thread %ld timeout.", curr_thd->tid);
    timer_set(&switch_timer, 1000);
    thread_schedule();
}
static void enter_thd(ThreadCtx *thd) {
    ThreadCtx *tmp_thd = curr_thd;
    ATOMIC { curr_thd = thd; }
    switch_to(tmp_thd, curr_thd);
}
static void to_zombie(ThreadCtx *thd) {
    ATOMIC { lln_push_back(&zombies_list, &thd->list); }
}

void idle() {
    timer_set(&switch_timer, 1000);
    while (1) {
        kill_zombies();
        thread_schedule();
    }
}

void init_thread() {
    _idle_thd.ra = (uintptr_t)idle;
    _idle_thd.sp = (uintptr_t)_idle_stack + sizeof(_idle_stack);
    memset(_idle_thd.sx, 0, sizeof(_idle_thd.sx));
    _idle_thd.k_stack = _idle_stack;
    _idle_thd.u_stack = NULL;
    lln_init(&_idle_thd.list);
    lln_init(&zombies_list);
    timer_add(&switch_timer, -1, _thd_timer_cb, NULL);
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
    lln_init(&ctx->wait_queue);

    ATOMIC { lln_push_back(idle_list, &ctx->list); }
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

    new_ctx->k_stack = k_stack;
    new_ctx->u_stack = u_stack;
    new_ctx->tid     = global_tid++;
    lln_init(&new_ctx->list);
    lln_init(&new_ctx->wait_queue);

    ATOMIC { lln_push_back(idle_list, &new_ctx->list); }
    return new_ctx->tid;
}
int thread_stop(long tid) {
    if (tid == curr_thd->tid)
        thread_exit();

    ATOMIC {
        LinkedListNode *tmp = idle_list->next;
        while (tmp != idle_list) {
            ThreadCtx *thd = container_of(tmp, ThreadCtx, list);
            if (thd->tid == tid) {
                lln_remove(tmp);
                to_zombie(thd);
                return 0;
            }
            tmp = tmp->next;
        }
    }
    return -1;
}

long thread_wait(long pid) {
    LinkedListNode *tmp;
    ATOMIC {
        tmp = zombies_list.next;
        while (tmp != &zombies_list) {
            ThreadCtx *thd = container_of(tmp, ThreadCtx, list);
            if (thd->tid == pid) {
                clean_thread(thd);
                return pid;
            }
            tmp = tmp->next;
        }
    }

    bool finded = false;
    ATOMIC {
        tmp = idle_list->next;
        while (tmp != idle_list) {
            ThreadCtx *thd = container_of(tmp, ThreadCtx, list);
            if (thd->tid == pid) {
                lln_push_back(&thd->wait_queue, &curr_thd->list);
                finded = true;
                break;
            }
            tmp = tmp->next;
        }
    }

    if (finded) {
        enter_thd(&_idle_thd);
        return pid;
    }
    return -1;
}

void thread_exit() {
    ATOMIC {
        LinkedListNode *tmp = curr_thd->wait_queue.next;
        while (tmp != &curr_thd->wait_queue) {
            ThreadCtx *thd = container_of(tmp, ThreadCtx, list);
            tmp            = tmp->next;
            lln_push_back(idle_list, &thd->list);
        }
        to_zombie(curr_thd);
        curr_thd = &_idle_thd;
    }
    switch_to(NULL, &_idle_thd);
}

void thread_schedule() {
    ThreadCtx *next_thd = NULL;
    ATOMIC {
        if (lln_empty(idle_list))
            return;

        LinkedListNode *next_node;
        next_node = lln_pop_front(idle_list);
        next_thd  = container_of(next_node, ThreadCtx, list);
        if (curr_thd != &_idle_thd)
            lln_push_back(idle_list, &curr_thd->list);
    }
    enter_thd(next_thd);
}

ThreadCtx *get_current() { return curr_thd; }
