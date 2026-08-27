# Lab 4

In this lab, we need to handle exception and interrupt.

## Exception

We can't let a random user program touch our system, so all system operation should be done by *operation system*.
If a user program needs some system functions, it needs to use "system call" to ask the OS for help.
To notify the OS, it will raise an *exception*, and the OS will handle this exception.
We can then add some parameters into the exceptions to represent different system calls.

The `ecall` (environment call) instruction can cause an exception, and a higher level process will handle it.
For example, we know there are three different permission levels in RISC-V.
When a program run under U-mode, the `ecall` will be cached by S-mode.

```txt
+--------------------------+
| U-mode (user mode)       |
+--------------------------+
| S-mode (supervisor mode) |
+--------------------------+
| M-mode (machine mode)    |
+--------------------------+
```

Under S-mode, after handling the system call, we can use `sret` (S-mode return) instruction to return to U-mode program.

## Interrupt

Interrupt is a mechanism that allows the hardware tell the OS something happen.
When a interrupt happened, the hardware will call a function we set automatically, then the OS can handle this events.

## Trap handling

Both `ecall` and interrupt will enter the same handling function called *trap handler*.

We have said that the hardware will call this function automatically,
so we need to configure it during initialization our trap handling system.
The hardware will jump to the address set in `stvec` register. so we save the handler function address into it.

In trap handler, we have to do following things:

1. Save context
2. Handle trap
3. Restore context and return

### Save Context

What is context?\
Context is the register data in the CPU when a program is running and other necessary registers.
In RISC-V, we have `x1 ~ x31` 31 registers in the CPU.

To return the correct address after trap handling, we also need to store the address when trap happened.
This address is saved in `sepc` (supervisor exception program counter) by hardware automatically.

The processor state and permission status also need to keep the same after return from trap handler.
Those data is stored in `sstatus` register.

So, our context for trap handler is `x1 ~ x31`, `sepc` and `sstatus`.
We need to save the value in this registers into the stack before calling the trap handling function.

### Handle Trap
