#ifndef _SHELL_H_
#define _SHELL_H_

// (func name, command name, description)
#define SHELL_FUNC_LIST                                                       \
    X(ls, ls, "list all files in ramdisk")                                    \
    X(cat, cat, "print file's contant")                                       \
    X(hello, hello, "print hello")                                            \
    X(help, help, "print this help")                                          \
    X(timeout, timeout, "timeout <num> <text>: show <text> after <num> sec.") \
    X(shexec, exec, "exec <path>: execution the binary file in <path>.")      \
    X(info, info, "print SBI information")

#define X(func, name, desc) int func(char *args);
SHELL_FUNC_LIST
#undef X

int shell(void);

#endif // !_SHELL_H_
