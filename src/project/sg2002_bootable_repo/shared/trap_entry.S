.section .text.trap
.align 2
.globl kraken_trap_entry
kraken_trap_entry:
    /* Encode CSR reads directly so rv64imac builds do not require zicsr in -march. */
    .word 0x34202573
    .word 0x341025f3
    .word 0x34302673
    .word 0x300026f3
    la t0, kraken_trap_panic
    jr t0
