.section .text

.globl riscv_read_misa
riscv_read_misa:
    /* Encode csrr a0, misa directly so rv64imac builds do not require zicsr. */
    .word 0x30102573
    ret

.globl riscv_read_mvendorid
riscv_read_mvendorid:
    .word 0xF1102573
    ret

.globl riscv_read_marchid
riscv_read_marchid:
    .word 0xF1202573
    ret

.globl riscv_read_mimpid
riscv_read_mimpid:
    .word 0xF1302573
    ret

.globl riscv_read_mhartid
riscv_read_mhartid:
    .word 0xF1402573
    ret
