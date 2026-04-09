.section .text.trap.supervisor
.align 2
.globl kraken_supervisor_trap_entry
kraken_supervisor_trap_entry:
    /* Encode CSR reads directly so rv64imac builds do not require zicsr in -march. */
    .word 0x14202573
    .word 0x141025f3
    .word 0x14302673
    .word 0x100026f3
    la t0, kraken_trap_panic
    jr t0
