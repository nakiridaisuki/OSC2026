#ifndef _SYSCALL_H_
#define _SYSCALL_H_

long sys_ecall(
    unsigned long num,
    unsigned long arg0,
    unsigned long arg1,
    unsigned long arg2,
    unsigned long arg3,
    unsigned long arg4,
    unsigned long arg5,
    unsigned long arg6
);

void init_syscall();

inline static long getpid() {
    /*
     * Return current process's pid.
     */
    return sys_ecall(0, 0, 0, 0, 0, 0, 0, 0);
}
inline static long uart_read(char *buf, long count) {
    /*
     * Read <count> bytes into buf.
     * Return the number of bytes read.
     */
    return sys_ecall(1, (long)buf, count, 0, 0, 0, 0, 0);
}
inline static long uart_write(const char *buf, long count) {
    /*
     * Write <count> bytes from buf.
     * Return the number of bytes written.
     */
    return sys_ecall(2, (long)buf, count, 0, 0, 0, 0, 0);
}
inline static int exec(const char *path) {
    /*
     * Load and exec program in path.
     * Return 0 on success, -1 on failure.
     */
    return sys_ecall(3, (long)path, 0, 0, 0, 0, 0, 0);
}
inline static long fork() {
    /*
     * Duplicate current process.
     * Return child's pid to parent, 0 to child.
     */
    return sys_ecall(4, 0, 0, 0, 0, 0, 0, 0);
}
inline static long waitpid(long pid) {
    /*
     * Wait the process with pid to finish.
     * Return pid of the finished process.
     */
    return sys_ecall(5, pid, 0, 0, 0, 0, 0, 0);
}
inline static void exit(int status) {
    /*
     * Terminate current process, use <status> to indicate the exit reason.
     */
    sys_ecall(6, status, 0, 0, 0, 0, 0, 0);
}
inline static int stop(long pid) {
    /*
     * Terminate the process with pid.
     * Return 0 on success, -1 on failure.
     */
    return sys_ecall(7, pid, 0, 0, 0, 0, 0, 0);
}
inline static int yield() {
    /*
     * Schedule to next idle thread.
     */
    return sys_ecall(8, 0, 0, 0, 0, 0, 0, 0);
}

#endif // !_SYSCALL_H_
