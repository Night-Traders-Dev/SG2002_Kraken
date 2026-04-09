#include "kraken.h"

void ctl_note_riscv_identity(shared_ctrl_t *ctl, uint32_t slot) {
    volatile kraken_riscv_identity_t *identity;

    if (slot >= RISCV_IDENTITY_SLOTS)
        return;

    identity = &ctl->riscv_identity[slot];
    identity->hartid = (uint32_t)riscv_read_mhartid();
    identity->mvendorid = (uint32_t)riscv_read_mvendorid();
    identity->marchid = (uint32_t)riscv_read_marchid();
    identity->mimpid = (uint32_t)riscv_read_mimpid();
    identity->misa = (uint32_t)riscv_read_misa();
    ctl_flush(ctl);
}

void ctl_note_riscv_boot_identity(shared_ctrl_t *ctl, uint32_t slot, uint32_t hartid) {
    volatile kraken_riscv_identity_t *identity;

    if (slot >= RISCV_IDENTITY_SLOTS)
        return;

    identity = &ctl->riscv_identity[slot];
    identity->hartid = hartid;
    identity->mvendorid = 0;
    identity->marchid = 0;
    identity->mimpid = 0;
    identity->misa = 0;
    ctl_flush(ctl);
}
