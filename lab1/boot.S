.section .text.entry
.global _start

_start:
    la sp, _stack_top

    la t0, _bss_start
    la t1, _bss_end

clear_bss_loop:
    bgeu t0, t1, bss_cleaned

    sd zero, 0(t0)
    addi t0, t0, 8

    j clear_bss_loop

bss_cleaned:
    call main

loop_forever:
    wfi
    j loop_forever

