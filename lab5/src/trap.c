#include "trap.h"
#include "malloc.h"
#include "printf.h"
#include "sbi.h"
#include "types.h"
#include <stddef.h>
#include <stdint.h>

#define MAX_LOCAL_INTR 16
#define MAX_EXCEPTIONS 64

extern void trap_entry(void);
static intr_handler_t local_intr_table[MAX_LOCAL_INTR];
static excep_handler_t exception_table[MAX_EXCEPTIONS];
static int current_priority = 999;

static void default_intr(void *context) {
    uint64_t scause, stval, sepc;
    asm volatile("csrr %0, scause" : "=r"(scause));
    asm volatile("csrr %0, stval" : "=r"(stval));
    asm volatile("csrr %0, sepc" : "=r"(sepc));
    printf(
        "[ERROR] Unhandled interrupt! scause: 0x%lx, sepc: 0x%lx, stval: 0x%lx\n",
        scause,
        sepc,
        stval
    );
    while (1) {
    }
}

static void default_excep(TrapFrame *tf, uint64_t stval) {
    uint64_t scause, sepc;
    asm volatile("csrr %0, scause" : "=r"(scause));
    asm volatile("csrr %0, stval" : "=r"(stval));
    asm volatile("csrr %0, sepc" : "=r"(sepc));
    printf(
        "[ERROR] Unhandled exception! scause: 0x%lx, sepc: 0x%lx, stval: 0x%lx\n",
        scause,
        sepc,
        stval
    );
    while (1) {
    }
}

typedef struct _task Task;
struct _task {
    int priority;
    callback_t callback;
    void *args;
    Task *next;
};
static Task task_head = {0, NULL, NULL, NULL};
void trap_add_task(callback_t callback, void *args, int priority) {
    Task *new_tsk = malloc(sizeof(Task));
    *new_tsk      = (Task){priority, callback, args};
    ATOMIC {
        Task *curr = &task_head;
        while (curr->next != NULL && curr->next->priority <= priority)
            curr = curr->next;
        new_tsk->next = curr->next;
        curr->next    = new_tsk;
    }
}

void register_local_intr(uint32_t code, intr_handler_t handler) {
    if (code < MAX_LOCAL_INTR)
        local_intr_table[code] = handler;
}
void register_exception(uint32_t code, excep_handler_t handler) {
    if (code < MAX_EXCEPTIONS)
        exception_table[code] = handler;
}

void init_trap() {
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

    for (size_t i = 0; i < MAX_LOCAL_INTR; i++)
        local_intr_table[i] = default_intr;
    for (size_t i = 0; i < MAX_EXCEPTIONS; i++)
        exception_table[i] = default_excep;
}

void trap_handler(TrapFrame *tf) {
    uint64_t scause, stval;
    asm volatile("csrr %0, scause" : "=r"(scause));
    asm volatile("csrr %0, stval" : "=r"(stval));

    // uint64_t sepc;
    // sepc = tf->sepc;
    // printf("\n[Kernel Trap Handler] Caught an exception!\n");
    // printf("  scause: 0x%lx\n", scause);
    // printf("  sepc:   0x%lx\n", sepc);
    // printf("  stval:  0x%lx\n", stval);

    if (scause & (1ULL << 63)) { // interrupt
        scause ^= (1ULL << 63);
        local_intr_table[scause](NULL);
    } else { // exception
        exception_table[scause](tf, stval);
    }

    while (1) {
        if (task_head.next == NULL || task_head.next->priority >= current_priority)
            break;

        Task *tsk      = task_head.next;
        task_head.next = tsk->next;

        int prev_prio    = current_priority;
        current_priority = tsk->priority;

        // enable supervisor global intr (SIE)
        asm volatile("csrs sstatus, 2");
        tsk->callback(tsk->args);
        free(tsk);
        // disable supervisor global intr (SIE)
        asm volatile("csrc sstatus, 2");
        current_priority = prev_prio;
    }
}
