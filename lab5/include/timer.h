#ifndef _TIMER_H_
#define _TIMER_H_

#include "dstruc.h"
#include "types.h"
#include <stdint.h>

typedef struct {
    uint64_t expires;
    callback_t callback;
    void *arg;
    LinkedListNode list;
} Timer;

static inline uint64_t __rdtime() {
    uint64_t curr_time;
    asm volatile("rdtime %0" : "=r"(curr_time));
    return curr_time;
}

void init_timer(const uint8_t *fdt);

void timer_add_us(Timer *timer, uint64_t delay_us, callback_t callback, void *arg);
static inline void timer_add(Timer *timer, uint64_t delay_ms, callback_t callback, void *arg) {
    timer_add_us(timer, 1000 * delay_ms, callback, arg);
}

void timer_set_us(Timer *timer, uint64_t delay_us);
static inline void timer_set(Timer *timer, uint64_t delay_ms) {
    timer_set_us(timer, delay_ms * 1000);
}

#endif // !_TIMER_H_
