#include "trap.h"
#include "cpio.h"
#include "dstruc.h"
#include "dtb.h"
#include "printf.h"
#include "sbi.h"
#include "timer.h"
#include "uart.h"
#include <stdint.h>
#include <stdlib.h>

phys_addr_t PLIC_BASE;

void init_trap(const uint8_t *fdt_ptr) {
    FDTHeader fdt_header          = get_fdt_header(fdt_ptr);
    const uint8_t *dt_struct_ptr  = fdt_ptr + fdt_header.off_dt_struct;
    const uint8_t *dt_strings_ptr = fdt_ptr + fdt_header.off_dt_strings;
    FDTProp reg =
        fdt_find_prop_by_path(dt_struct_ptr, dt_strings_ptr, "/soc/interrupt-controller", "reg");
    PLIC_BASE = fdt_read_u64_save(reg, 0);

    // set trap handler enter point
    asm volatile("csrw stvec, %0" : : "r"((uint64_t)trap_entry));

    // init interrupt sp reg to 0
    asm volatile("csrw sscratch, zero");

    // enable supervisor global intr (SIE)
    asm volatile("csrs sstatus, 2");

    // enable supervisor time intr (STIE)
    sbi_set_timer(-1);
    asm volatile("csrs sie, %0" ::"r"(1UL << 5));

    // enable supervisor external intr (SEIE)
    asm volatile("csrs sie, %0" ::"r"(1UL << 9));
}

void trap_handler(TrapFrame *tf) {
    uint64_t scause, stval, sepc;
    asm volatile("csrr %0, scause" : "=r"(scause));
    asm volatile("csrr %0, stval" : "=r"(stval));
    sepc = tf->sepc;

    // printf("\n[Kernel Trap Handler] Caught an exception!\n");
    // printf("  scause: 0x%lx\n", scause);
    // printf("  sepc:   0x%lx\n", sepc);
    // printf("  stval:  0x%lx\n", stval);

    if (scause & (1ULL << 63)) { // interrupt
        scause ^= (1ULL << 63);

        if (scause == 5) { // timer interrupt
            uint64_t now = __rdtime();
            while (MIN_TIMER != &TIMER_LIST_HEAD && MIN_TIMER->expires <= now) {
                Timer *timer = MIN_TIMER;
                lln_remove(&timer->list);

                if (timer->callback)
                    timer->callback(timer->arg);

                free(timer);
            }
            sbi_set_timer(MIN_TIMER->expires);
        } else if (scause == 9) { // external interrupt
            while (1) {
                uint32_t irq = PLIC_CLAME(1);
                if (irq == 0)
                    break;

                if (irq == UART_IRQ) {
                    uart_intr_handle();
                } else {
                    printf("Unknow irq 0x%x\n", irq);
                    while (1) {
                    }
                }

                PLIC_COMPLETE(1, irq);
            }
        } else
            while (1) {
            }

    } else { // exception
        if (scause == 8) {
            tf->sepc += 4;
            printf("sepc += 4\n");
        } else
            while (1) {
            }
    }
}
