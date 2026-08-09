// Global entry point
.section .text.entry
.global _start 

_start:
    // load current address
    lla t0, _start 
    
    // load expected start address
    lla t4, _link_start_ptr
    ld t1, 0(t4)

    // already at expected start address
    beq t0, t1, relocation_done

    // load expected end address
    lla t4, _link_end_ptr
    ld t2, 0(t4)

relocation_loop:
    // copy 8 byte to expected address
    ld t3, 0(t0)
    sd t3, 0(t1)

    // shift 8 byte
    addi t0, t0, 8
    addi t1, t1, 8

    // check end
    bne t1, t2, relocation_loop

    fence.i

    // jump to new address
    lla t4, _relocation_done_ptr
    ld t0, 0(t4)
    jr t0

relocation_done:
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

.align 3
_link_start_ptr:
    .quad _start_address
_link_end_ptr:
    .quad _end_address
_relocation_done_ptr:
    .quad relocation_done
