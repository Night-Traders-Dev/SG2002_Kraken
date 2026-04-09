#include "kraken.h"

void ctl_flush(shared_ctrl_t *ctl) {
    flush_dcache_range((uintptr_t)ctl, (uintptr_t)ctl + sizeof(*ctl));
}

void ctl_invalidate(shared_ctrl_t *ctl) {
    inval_dcache_range((uintptr_t)ctl, (uintptr_t)ctl + sizeof(*ctl));
}

void ctl_init_defaults(shared_ctrl_t *ctl) {
    uint32_t boot_count = 0;

    if (ctl->magic == KRAKEN_MAGIC && ctl->version == KRAKEN_VERSION)
        boot_count = ctl->boot_count;

    sg2002_memset((void *)ctl, 0, sizeof(*ctl));
    ctl->magic = KRAKEN_MAGIC;
    ctl->version = KRAKEN_VERSION;
    ctl->system_stage = STAGE_COLD_RESET;
    ctl->kernel_entry_addr = (uint32_t)KERNEL_LOAD_ADDR;
    ctl->worker_entry_addr = (uint32_t)WORKER_LOAD_ADDR;
    ctl->worker_state = CORE_OFFLINE;
    ctl->worker_cmd = CMD_NONE;
    ctl->usb_state = USB_SERIAL_OFF;
    ctl->usb_console_enabled = KRAKEN_ENABLE_USB_DWC2_SCAFFOLD ? 1u : 0u;
    ctl->platform_caps = sg2002_platform_caps();
    ctl->boot_count = boot_count + 1u;
    if (ctl->boot_count == 0)
        ctl->boot_count = 1u;
    ctl_flush(ctl);
}

void ctl_set_stage(shared_ctrl_t *ctl, uint32_t stage) {
    ctl->system_stage = stage;
    ctl_flush(ctl);
}

uint32_t ctl_next_cmd_seq(shared_ctrl_t *ctl) {
    ctl->kernel_cmd_seq++;
    ctl_flush(ctl);
    return ctl->kernel_cmd_seq;
}

void ctl_fault_log(shared_ctrl_t *ctl, uint32_t tag, uint32_t code,
                   uint32_t arg0, uint32_t arg1) {
    uint32_t slot = ctl->fault_log_head & (KRAKEN_FAULT_LOG_SIZE - 1u);

    ctl->fault_log[slot].tag = tag;
    ctl->fault_log[slot].code = code;
    ctl->fault_log[slot].arg0 = arg0;
    ctl->fault_log[slot].arg1 = arg1;
    ctl->fault_last_tag = tag;
    ctl->fault_last_code = code;
    ctl->fault_last_arg0 = arg0;
    ctl->fault_last_arg1 = arg1;
    ctl->fault_log_head = (slot + 1u) & (KRAKEN_FAULT_LOG_SIZE - 1u);
    if (ctl->fault_log_count < KRAKEN_FAULT_LOG_SIZE)
        ctl->fault_log_count++;
    ctl_flush(ctl);
}

void ctl_note_trap(shared_ctrl_t *ctl, uint32_t source_tag,
                   uint64_t mcause, uint64_t mepc,
                   uint64_t mtval, uint64_t mstatus) {
    uint32_t packed_mcause = (uint32_t)(mcause & 0x7fffffffu);

    if ((mcause >> 63) != 0)
        packed_mcause |= 0x80000000u;

    ctl->trap_count++;
    ctl->trap_last_source = source_tag;
    ctl->trap_last_cause = packed_mcause;
    ctl->trap_last_epc = (uint32_t)mepc;
    ctl->trap_last_tval = (uint32_t)mtval;
    ctl->trap_last_status = (uint32_t)mstatus;
    ctl_fault_log(ctl, source_tag, packed_mcause,
                  (uint32_t)mepc, (uint32_t)mtval);
}

void ctl_set_platform_error(shared_ctrl_t *ctl, uint32_t error_mask) {
    ctl->platform_errors |= error_mask;
    ctl_flush(ctl);
}

void ctl_clear_platform_error(shared_ctrl_t *ctl, uint32_t error_mask) {
    ctl->platform_errors &= ~error_mask;
    ctl_flush(ctl);
}
