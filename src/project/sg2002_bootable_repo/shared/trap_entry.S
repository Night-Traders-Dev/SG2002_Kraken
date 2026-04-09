.section .bss.trap_stack,"aw",@nobits
.align 4
kraken_trap_stack:
    .space 256
kraken_trap_stack_top:

.section .text.trap
.align 2
.globl kraken_trap_entry
kraken_trap_entry:
    /* Switch to a dedicated trap stack before touching any other state. */
    .word 0x34011073   /* csrw mscratch, sp */
    la sp, kraken_trap_stack_top
    /* Encode CSR reads directly so rv64imac builds do not require zicsr in -march. */
    .word 0x34202573   /* csrr a0, mcause  */
    .word 0x341025f3   /* csrr a1, mepc    */
    .word 0x34302673   /* csrr a2, mtval   */
    .word 0x300026f3   /* csrr a3, mstatus */
    la t0, kraken_trap_panic
    jr t0
