#include "timer.h"
#include "dstruc.h"
#include "dtb.h"
#include "printf.h"
#include "sbi.h"
#include "trap.h"
#include <stdint.h>

uint64_t HZ_PER_SEC;
Timer TIMER_LIST_HEAD;

void init_timer(uint8_t *fdt_ptr) {
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
