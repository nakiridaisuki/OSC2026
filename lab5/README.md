# Lab 5

In this lab, we will implement the thread.

*I won't distinguish between threads and processes in this lab, so I may use both words.*

## Thread

The thread/process is a running entity of a program in the memory.
Basically, a process should run under user mode, and switch to kernel mode via system call if needed.
The stander workflow to create a new process and run some program is `fork` current process and `exec` the target program.

```
[current proc] <U-mode>
       | fork
[current proc] <K-mode>
       | create a new process
       |______________________
       |                      |
[current proc] <K-mode>   [new proc] <K-mode>
       | sret                 | sret
  keep do something       [new proc] <U-mode>
                              | exec
                          [new proc] <K-mode>
                              | kernel load program
                              | sret
                          [target program] <U-mode>

// U-mode for user mode
// K-mode for kernel mode
```

To control a thread, we need to maintain some necessary information of it.
A user process need a memory area for program code, so we need to allocate for it.
Also, we need a stack for kernel mode.

```txt
Process Control Block:
kernel stack 
user program space
```

and we need some saved data for context switch, like:

```txt
Process Control Block:
...
ra: return address
sp: stack pointer
s[0~11]: saved registers
```

and some other control data like timer or signal handler.

```txt
Process Control Block:
...
linked list node
timer
signal control block
```

This data structure is called *Process control block* commonly, or PCB for short.

## Scheduler

Our kernel should be able to schedule between multiple threads.

```txt
[thd 1 working] --for some reason--> [schedule] --thread switch--> [thd 2 working]
```

When thread 1 be scheduled back, we hope it keep doing what it did before scheduled.
The term 'keep doing' here in program execution means to run the next instruction.

To schedule between threads, we can use the `ra`(return address) register to achieve it.

When the process meet a `ret` instruction, it will change current `pc` to the value in `ra`.
We can write a function for thread switching using this mechanism.
When we call this function, the `ra` will be set to next line of it by the `call` instruction.
In this switching function, we set `ra` to target thread's saved `ra` and call `ret`.
Then our `pc` will at the desired position of target process.

Thus, the switch function should like this:

```asm
.globl switch_to

switch_to:
    sd ra, 8*0(a0) // save current ra
    sd sp, 8*1(a0)
    sd s0, 8*2(a0)
    ...
    sd s11, 8*13(a0)

    ld ra, 8*0(a1) // load target ra
    ld sp, 8*1(a1)
    ld s0, 8*2(a1)
    ...
    ld s11, 8*13(a1)

    move tp, a1
    ret // jump to target ra
```

Above things are run under *kernel mode*.

## Idle Thread

When there are no thread in ready queue, we need a always runnable thread for CPU to execute next instruction.
Also, the idle thread can be a schedule center.
When a thread exit, it can switch to the idle thread, and it will keep schedule for next ready thread.

## System Calls

After knowing how to switch between processes, we need to figure out how to switch between *user* processes.

The scheduling is happened under kernel mode,
and the `fork`, `exec` and other functions offered by kernel is also run under kernel mode.
So we need the `ecall` instruction which introduced in Lab 4 to switch from user mode to kernel mode.
This `ecall`'s argument can be defined by us, and we call *system call* for this types of function calls.
