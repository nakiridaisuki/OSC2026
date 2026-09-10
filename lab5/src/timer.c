#include "timer.h"
#include "dstruc.h"
#include "dtb.h"
#include "malloc.h"
#include "printf.h"
#include "sbi.h"
#include "trap.h"
#include "types.h"
#include <stdint.h>

#define NODE_TO_TIMER(nodeptr) container_of(nodeptr, Timer, list)
#define MIN_TIMER              container_of(TIMER_LIST_HEAD.list.prev, Timer, list)
#define MAX_TIMER              container_of(TIMER_LIST_HEAD.list.next, Timer, list)

static uint64_t HZ_PER_SEC;
static Timer TIMER_LIST_HEAD;

static void timer_intr_handler(void *context) {
    uint64_t now = __rdtime();
    while (MIN_TIMER != &TIMER_LIST_HEAD && MIN_TIMER->expires <= now) {
        Timer *timer = MIN_TIMER;
        lln_remove(&timer->list);
        if (timer->callback)
            trap_add_task(timer->callback, timer->arg, 5);
    }
    sbi_set_timer(MIN_TIMER->expires);
}

void init_timer(const uint8_t *fdt_ptr) {
    FDTHeader fdt_header          = get_fdt_header(fdt_ptr);
    const uint8_t *dt_struct_ptr  = fdt_ptr + fdt_header.off_dt_struct;
    const uint8_t *dt_strings_ptr = fdt_ptr + fdt_header.off_dt_strings;

    FDTProp freq =
        fdt_find_prop_by_path(dt_struct_ptr, dt_strings_ptr, "/cpus", "timebase-frequency");

    uint64_t clock = fdt_read_num_save(freq, 0);
    if (clock == 0) {
        printf("Error: can't get cpu clock from FDT.\n");
        return;
    }
    HZ_PER_SEC = clock;

    TIMER_LIST_HEAD.expires  = -1;
    TIMER_LIST_HEAD.callback = TIMER_LIST_HEAD.arg = NULL;
    lln_init(&TIMER_LIST_HEAD.list);
    register_local_intr(5, timer_intr_handler);
}

void timer_add(Timer *timer, uint64_t delay_ms, callback_t callback, void *arg) {
    timer->expires  = __rdtime() + delay_ms * HZ_PER_SEC / 1000;
    timer->callback = callback;
    timer->arg      = arg;
    lln_init(&timer->list);

    ATOMIC {
        Timer *tmp = MAX_TIMER;
        while (tmp->expires > timer->expires) {
            if (tmp == &TIMER_LIST_HEAD)
                break;
            tmp = NODE_TO_TIMER(tmp->list.next);
        }
        lln_add(tmp->list.prev, &timer->list);
        if (tmp == &TIMER_LIST_HEAD)
            sbi_set_timer(timer->expires);
    }
}
void timer_set(Timer *timer, uint64_t delay_ms) {
    ATOMIC {
        int finded = 0;
        Timer *tmp = MIN_TIMER;
        while (tmp != &TIMER_LIST_HEAD) {
            if (tmp == timer) {
                finded = 1;
                break;
            }
            tmp = NODE_TO_TIMER(tmp->list.next);
        }
        if (finded) {
            lln_remove(&timer->list);
        }
    }

    timer->expires = __rdtime() + delay_ms * HZ_PER_SEC / 1000;

    ATOMIC {
        Timer *tmp = MAX_TIMER;
        while (tmp->expires > timer->expires) {
            if (tmp == &TIMER_LIST_HEAD)
                break;
            tmp = NODE_TO_TIMER(tmp->list.next);
        }
        lln_add(tmp->list.prev, &timer->list);
        if (tmp == &TIMER_LIST_HEAD)
            sbi_set_timer(timer->expires);
    }
}
