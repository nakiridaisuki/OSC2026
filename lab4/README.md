# Lab 4

In this lab, we need to handle exception and interrupt.

## Exception

We can't let a random user program touch our system, so all system operation should be done by "operation system".
If a user program need some system level functions, it need to use "system call" to ask OS for help.
When a user program needs OS help, it will raise a "exception", then the OS will handle this exception.
We can add some parameters into the exceptions to represent different system calls.

The `ecall` (environment call) instruction can cause a exception, then a higher level process will handle it.
For example, we know there are three different permission level in RISC-V. From low to high:

```txt
+--------------------------+
| U-mode (user mode)       |
+--------------------------+
| S-mode (supervisor mode) |
+--------------------------+
| M-mode (machine mode)    |
+--------------------------+
```

If we U-mode
