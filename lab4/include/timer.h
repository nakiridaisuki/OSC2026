#ifndef _TIMER_H_
#define _TIMER_H_

#include "dstruc.h"
#include <stdint.h>

typedef void (*timer_cb_t)(void *arg);

typedef struct {
    uint64_t expires;
    timer_cb_t callback;
    void *arg;
    LinkedListNode list;
} Timer;

static inline uint64_t __rdtime() {
    uint64_t curr_time;
    asm volatile("rdtime %0" : "=r"(curr_time));
    return curr_time;
}

extern uint64_t HZ_PER_SEC;
extern Timer TIMER_LIST_HEAD;

#define NODE_TO_TIMER(nodeptr) container_of(nodeptr, Timer, list)
#define MIN_TIMER              container_of(TIMER_LIST_HEAD.list.prev, Timer, list)
#define MAX_TIMER              container_of(TIMER_LIST_HEAD.list.next, Timer, list)

void init_timer(uint8_t *fdt);
void add_timer(Timer *timer, uint64_t delay_ms, timer_cb_t callback, void *arg);

#endif // !_TIMER_H_
