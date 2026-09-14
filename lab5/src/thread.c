#include "thread.h"
#include "dstruc.h"
#include "malloc.h"
#include "printf.h"
#include "string.h"
#include "timer.h"
#include "trap.h"
#include <stdint.h>

#define EXPIRE (1000 / 32)

extern int switch_to(ThreadCtx *prev, ThreadCtx *next);
extern int trap_restore();

static long global_tid = 0;
static ThreadCtx _idle_thd;
static ThreadCtx *curr_thd = &_idle_thd;
static uint8_t _idle_stack[128];

static LinkedListNode *idle_list = &_idle_thd.list;
static LinkedListNode zombies_list, sleep_list;

static Timer switch_timer;

static void clean_thread(ThreadCtx *thd) {
    if (thd->k_stack)
        free(thd->k_stack);
    if (thd->u_space)
        free(thd->u_space);
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
    thread_schedule();
}
static void enter_thd(ThreadCtx *thd) {
    ThreadCtx *tmp_thd = curr_thd;
    ATOMIC { curr_thd = thd; }
    switch_to(tmp_thd, curr_thd);
}
static void to_zombie(ThreadCtx *target) {
    ATOMIC {
        LinkedListNode *tmp = target->wait_queue.next;
        while (tmp != &target->wait_queue) {
            ThreadCtx *thd = container_of(tmp, ThreadCtx, list);
            tmp            = tmp->next;
            lln_push_back(idle_list, &thd->list);
        }
        lln_push_back(&zombies_list, &target->list);
    }
}
static void _init_thd(ThreadCtx *ctx, void *k_stack, void *u_space, uint64_t u_len) {
    ctx->k_stack = k_stack;
    ctx->u_space = u_space;
    ctx->u_len   = u_len;
    ctx->tid     = global_tid++;
    ctx->stat    = AVAIL;

    ctx->pending_signal = 0;
    ctx->in_signal_hdlr = 0;
    ctx->signal_stack   = NULL;

    lln_init(&ctx->list);
    lln_init(&ctx->wait_queue);
}

void idle() {
    timer_set(&switch_timer, EXPIRE);
    while (1) {
        kill_zombies();
        if (lln_empty(idle_list)) {
            intr_restore(1);
            asm volatile("wfi");
            intr_restore(0);
        }
        thread_schedule();
    }
}

void init_thread() {
    _idle_thd.ra = (uintptr_t)idle;
    _idle_thd.sp = (uintptr_t)_idle_stack + sizeof(_idle_stack);
    memset(_idle_thd.sx, 0, sizeof(_idle_thd.sx));
    _init_thd(&_idle_thd, NULL, NULL, 0);

    lln_init(&zombies_list);
    lln_init(&sleep_list);
    timer_add(&switch_timer, -1, _thd_timer_cb, NULL);
}

void thread_create(void (*func)(void)) {
    ThreadCtx *ctx = (ThreadCtx *)malloc(sizeof(ThreadCtx));
    memset(ctx, 0, sizeof(ThreadCtx));
    void *th_stack = malloc(4096);
    memset(th_stack, 0, 4096);

    ctx->ra = (uintptr_t)func;
    ctx->sp = (uintptr_t)th_stack + 4096;
    memset(ctx->sx, 0, sizeof(ctx->sx));
    _init_thd(ctx, th_stack, NULL, 0);
    ATOMIC { lln_push_back(idle_list, &ctx->list); }
}

long thread_fork(TrapFrame *tf) {
    ThreadCtx *new_ctx = (ThreadCtx *)malloc(sizeof(ThreadCtx));
    memset(new_ctx, 0, sizeof(ThreadCtx));

    // Handle kernel stack
    // put trap frame to the kernel stack of new thread
    void *k_stack = malloc(4096);
    new_ctx->ra   = (uintptr_t)trap_restore;
    new_ctx->sp   = (uint64_t)k_stack + 4096 - sizeof(TrapFrame);
    memcpy((char *)new_ctx->sp, tf, sizeof(TrapFrame));
    TrapFrame *new_tf = (TrapFrame *)new_ctx->sp;

    // Handle user memory space
    // copy full user memory and update necessery value in trap frame of new thread
    size_t u_len       = curr_thd->u_len;
    size_t u_stack_len = (char *)curr_thd->u_space + u_len - (char *)tf->sp;
    char *new_u_space  = malloc(u_len);
    memcpy(new_u_space, curr_thd->u_space, u_len);
    new_tf->sp = (uint64_t)(new_u_space + u_len - u_stack_len);
    new_tf->a0 = 0;

    for (int i = 0; i < MAX_SIGNAL; i++)
        new_ctx->signal_hdlr[i] = curr_thd->signal_hdlr[i];

    _init_thd(new_ctx, k_stack, new_u_space, u_len);
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

        tmp = sleep_list.next;
        while (tmp != &sleep_list) {
            ThreadCtx *thd = container_of(tmp, ThreadCtx, list);
            if (thd->tid == tid) {
                thd->stat = KILLED;
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

static void _thd_sleep_cb(void *args) {
    ThreadCtx *sleep_thd = (ThreadCtx *)args;
    ATOMIC { lln_remove(&sleep_thd->list); }
    if (sleep_thd->stat == KILLED)
        thread_exit();
    ATOMIC { lln_push_back(idle_list, &sleep_thd->list); }
}
int thread_sleep(unsigned int usec) {
    timer_add_us(&curr_thd->timer, usec, _thd_sleep_cb, curr_thd);
    ATOMIC { lln_push_back(&sleep_list, &curr_thd->list); }
    enter_thd(&_idle_thd);
    return 0;
}

void thread_exit() {
    to_zombie(curr_thd);
    ATOMIC { curr_thd = &_idle_thd; }
    switch_to(NULL, &_idle_thd);
}

void thread_schedule() {
    timer_set(&switch_timer, EXPIRE);
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

ThreadCtx *get_thd(long pid) {
    ATOMIC {
        LinkedListNode *tmp = idle_list->next;
        while (tmp != idle_list) {
            ThreadCtx *thd = container_of(tmp, ThreadCtx, list);
            if (thd->tid == pid)
                return thd;

            LinkedListNode *ttmp = thd->wait_queue.next;
            while (ttmp != &thd->wait_queue) {
                ThreadCtx *tthd = container_of(ttmp, ThreadCtx, list);
                if (tthd->tid == pid)
                    return tthd;
                ttmp = ttmp->next;
            }

            tmp = tmp->next;
        }

        tmp = zombies_list.next;
        while (tmp != &zombies_list) {
            ThreadCtx *thd = container_of(tmp, ThreadCtx, list);
            if (thd->tid == pid)
                return thd;
            tmp = tmp->next;
        }

        tmp = sleep_list.next;
        while (tmp != &sleep_list) {
            ThreadCtx *thd = container_of(tmp, ThreadCtx, list);
            if (thd->tid == pid)
                return thd;
            tmp = tmp->next;
        }
    }
    return NULL;
}
