// Global entry point
.section .text.entry
.global _start 

_start:
    // Get stack top address from linker script
    la sp, _stack_top 

    // Get bss start/end from linker script
    la t0, _bss_start
    la t1, _bss_end

// Clean for loop
clear_bss_loop:
    bgeu t0, t1, bss_cleaned

    sd zero, 0(t0)
    addi t0, t0, 8

    j clear_bss_loop

// Call the main function
bss_cleaned:
    call main

// Infinity loop if main return
loop_forever:
    wfi
    j loop_forever

