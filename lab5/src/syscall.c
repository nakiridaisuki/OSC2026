#include "syscall.h"
#include "cpio.h"
#include "malloc.h"
#include "printf.h"
#include "sbi.h"
#include "string.h"
#include "thread.h"
#include "trap.h"
#include "uart.h"

long sys_ecall(
    unsigned long num,
    unsigned long arg0,
    unsigned long arg1,
    unsigned long arg2,
    unsigned long arg3,
    unsigned long arg4,
    unsigned long arg5,
    unsigned long arg6
) {
    register unsigned long a0 asm("a0") = arg0;
    register unsigned long a1 asm("a1") = arg1;
    register unsigned long a2 asm("a2") = arg2;
    register unsigned long a3 asm("a3") = arg3;
    register unsigned long a4 asm("a4") = arg4;
    register unsigned long a5 asm("a5") = arg5;
    register unsigned long a6 asm("a6") = arg6;
    register unsigned long a7 asm("a7") = num;

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                 : "memory");
    return a0;
}

static long _uart_read(char *buf, long cnt) {
    intr_restore(1); // enable intruption for output
    long total = 0;
    for (size_t i = 0; i < cnt; i++) {
        buf[i] = uart_getchar();
        total++;
    }
    intr_restore(0); // disable intruption
    return total;
}

static long _uart_write(const char *buf, long cnt) {
    intr_restore(1); // enable intruption for output
    // printf("%s", buf);
    // return 0;
    long total = 0;
    for (size_t i = 0; i < cnt; i++) {
        if (UART_INIT_DONE)
            uart_putchar(buf[i]);
        else
            sbi_putchar(buf[i]);
        total++;
    }
    intr_restore(0); // disable intruption
    return total;
}

static void _exec(TrapFrame *tf) {
    CPIOFile file;
    const char *path = (const char *)tf->a0;
    if (!cpionewc_find(&file, path)) {
        printf("ERROR: Can't exec file %s: file not found.", path);
        return;
    }

    void *start_addr = malloc(file.header.filesize + 16 * 1024);
    memcpy(start_addr, file.data, file.header.filesize);
    memset(tf->regs, 0, sizeof(uintptr_t) * 32);
    tf->sp   = (uint64_t)start_addr + (file.header.filesize + 16 * 1024);
    tf->sepc = (uint64_t)start_addr;
}

static void syscall_hdlr(TrapFrame *tf, uint64_t stval) {
    tf->sepc += 4;
    long call_id = tf->a7;
    switch (call_id) {
    case 0: // getpid()
        tf->a0 = get_current()->tid;
        break;
    case 1: // uart_read(char* buf, long cnt)
        tf->a0 = _uart_read((char *)tf->a0, tf->a1);
        break;
    case 2: // uart_write(const char* buf, long cnt)
        tf->a0 = _uart_write((const char *)tf->a0, tf->a1);
        break;
    case 3: // exec(const char* path)
        _exec(tf);
        break;
    case 4: // fork()
        tf->a0 = thread_fork(tf);
        break;
    case 5: // waitpid(long pid)
        thread_wait(tf->a0);
        break;
    case 6: // exit(int status)
        thread_exit();
        break;
    case 7: // stop(long pid)
        thread_stop(tf->a0);
        break;
    case 8: // yield()
        thread_schedule();
        break;
    default:
        break;
    }
}

void init_syscall() { register_exception(8, syscall_hdlr); }
