#include "timer.h"
#include "dstruc.h"
#include "dtb.h"
#include "malloc.h"
#include "printf.h"
#include "sbi.h"
#include "trap.h"
#include <stdint.h>

#define NODE_TO_TIMER(nodeptr) container_of(nodeptr, Timer, list)
#define MIN_TIMER              container_of(TIMER_LIST_HEAD.list.prev, Timer, list)
#define MAX_TIMER              container_of(TIMER_LIST_HEAD.list.next, Timer, list)

static uint64_t HZ_PER_SEC;
static Timer TIMER_LIST_HEAD;

static void timer_intr_handler(uintptr_t sepc, uintptr_t stval, void *context) {
    uint64_t now = __rdtime();
    while (MIN_TIMER != &TIMER_LIST_HEAD && MIN_TIMER->expires <= now) {
        Timer *timer = MIN_TIMER;
        lln_remove(&timer->list);

        if (timer->callback)
            timer->callback(timer->arg);
        free(timer);
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

void add_timer(Timer *timer, uint64_t delay_ms, timer_cb_t callback, void *arg) {
    timer->expires  = __rdtime() + delay_ms * HZ_PER_SEC / 1000;
    timer->callback = callback;
    timer->arg      = arg;
    lln_init(&timer->list);

    uint64_t flag = intr_save_and_disable();

    Timer *tmp = MAX_TIMER;
    while (tmp->expires > timer->expires) {
        if (tmp == &TIMER_LIST_HEAD)
            break;
        tmp = NODE_TO_TIMER(tmp->list.next);
    }
    lln_add(tmp->list.prev, &timer->list);
    if (tmp == &TIMER_LIST_HEAD)
        sbi_set_timer(timer->expires);

    intr_restore(flag);
}
