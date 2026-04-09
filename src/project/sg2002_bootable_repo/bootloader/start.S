.section .text.start
.globl _start
_start:
    mv s0, a0
    mv s1, a1
    la sp, __stack_top
    la t0, kraken_supervisor_trap_entry
    /* Encode csrw stvec, t0 directly so rv64imac builds do not require zicsr. */
    .word 0x10529073
    la t0, __bss_start
    la t1, __bss_end
1:
    bgeu t0, t1, 2f
    sd zero, 0(t0)
    addi t0, t0, 8
    j 1b
2:
    mv a0, s0
    mv a1, s1
    call bootloader_main
3:  j 3b
