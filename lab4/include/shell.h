#ifndef _SHELL_H_
#define _SHELL_H_

// (return type, name, arguments, description)
#define SHELL_FUNC_LIST                                              \
    X(ls, "list all files in ramdisk")                               \
    X(cat, "print file's contant")                                   \
    X(hello, "print hello")                                          \
    X(help, "print this help")                                       \
    X(timeout, "timeout <num> <text>: show <text> after <num> sec.") \
    X(info, "print SBI information")

#define X(name, desc) int name(char *args);
SHELL_FUNC_LIST
#undef X

int shell(void);

#endif // !_SHELL_H_
