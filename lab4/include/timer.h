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

void init_timer(const uint8_t *fdt);
void add_timer(Timer *timer, uint64_t delay_ms, timer_cb_t callback, void *arg);

#endif // !_TIMER_H_
