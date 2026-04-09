.section .bss.trap_stack.supervisor,"aw",@nobits
.align 4
kraken_supervisor_trap_stack:
    .space 256
kraken_supervisor_trap_stack_top:

.section .text.trap.supervisor
.align 2
.globl kraken_supervisor_trap_entry
kraken_supervisor_trap_entry:
    /* Switch to a dedicated supervisor trap stack to survive sp corruption. */
    .word 0x14011073   /* csrw sscratch, sp */
    la sp, kraken_supervisor_trap_stack_top
    /* Encode CSR reads directly so rv64imac builds do not require zicsr in -march. */
    .word 0x14202573   /* csrr a0, scause  */
    .word 0x141025f3   /* csrr a1, sepc    */
    .word 0x14302673   /* csrr a2, stval   */
    .word 0x100026f3   /* csrr a3, sstatus */
    la t0, kraken_trap_panic
    jr t0
