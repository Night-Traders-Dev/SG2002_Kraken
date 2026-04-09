#include "kraken.h"

static void persist_invalidate(kraken_persist_log_t *log) {
    inval_dcache_range((uintptr_t)log, (uintptr_t)log + sizeof(*log));
}

static void persist_flush_header(kraken_persist_log_t *log) {
    flush_dcache_range((uintptr_t)log,
                       (uintptr_t)log + offsetof(kraken_persist_log_t, records));
}

static void persist_flush_record(kraken_persist_log_t *log, uint32_t slot) {
    flush_dcache_range((uintptr_t)&log->records[slot],
                       (uintptr_t)&log->records[slot] + sizeof(log->records[slot]));
}

static int persist_valid(const kraken_persist_log_t *log) {
    return log->magic == KRAKEN_PERSIST_MAGIC &&
           log->version == KRAKEN_PERSIST_VERSION;
}

static kraken_persist_log_t *persist_ensure(void) {
    kraken_persist_log_t *log = persistent_log();

    persist_invalidate(log);
    if (persist_valid(log))
        return log;

    sg2002_memset((void *)log, 0, sizeof(*log));
    log->magic = KRAKEN_PERSIST_MAGIC;
    log->version = KRAKEN_PERSIST_VERSION;
    persist_flush_header(log);
    return log;
}

static void persist_append(uint32_t boot_count, uint32_t kind, uint32_t source,
                           uint32_t code, uint32_t arg0, uint32_t arg1,
                           uint32_t stage) {
    kraken_persist_log_t *log = persist_ensure();
    uint32_t slot = log->write_head & (KRAKEN_PERSIST_LOG_CAPACITY - 1u);
    uint32_t seq = log->next_seq + 1u;

    log->next_seq = seq;
    log->records[slot].seq = seq;
    log->records[slot].boot_count = boot_count;
    log->records[slot].kind = kind;
    log->records[slot].source = source;
    log->records[slot].code = code;
    log->records[slot].arg0 = arg0;
    log->records[slot].arg1 = arg1;
    log->records[slot].stage = stage;
    log->write_head = (slot + 1u) & (KRAKEN_PERSIST_LOG_CAPACITY - 1u);
    if (log->record_count < KRAKEN_PERSIST_LOG_CAPACITY)
        log->record_count++;

    persist_flush_record(log, slot);
    persist_flush_header(log);
}

static void persist_note_previous_boot(const shared_ctrl_t *ctl) {
    kraken_persist_log_t *log;

    if (ctl->boot_count == 0u)
        return;

    log = persist_ensure();
    log->last_boot_count = ctl->boot_count;
    log->last_reset_reason = ctl->reset_reason;
    log->last_stage = ctl->system_stage;
    log->last_system_flags = ctl->system_flags;
    log->last_trace_source = ctl->trace_last_source;
    log->last_trace_code = ctl->trace_last_code;
    log->last_fault_tag = ctl->fault_last_tag;
    log->last_fault_code = ctl->fault_last_code;
    persist_flush_header(log);

    persist_append(ctl->boot_count, PERSIST_EVT_BOOT_SUMMARY,
                   ctl->trace_last_source, ctl->reset_reason,
                   ctl->system_stage, ctl->system_flags,
                   ctl->system_stage);
}

void ctl_flush(shared_ctrl_t *ctl) {
    flush_dcache_range((uintptr_t)ctl, (uintptr_t)ctl + sizeof(*ctl));
}

void ctl_invalidate(shared_ctrl_t *ctl) {
    inval_dcache_range((uintptr_t)ctl, (uintptr_t)ctl + sizeof(*ctl));
}

void ctl_init_defaults(shared_ctrl_t *ctl) {
    uint32_t boot_count = 0;

    ctl_invalidate(ctl);
    if (ctl->magic == KRAKEN_MAGIC && ctl->version == KRAKEN_VERSION) {
        boot_count = ctl->boot_count;
        persist_note_previous_boot(ctl);
    }

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
    persist_append(ctl->boot_count, PERSIST_EVT_STAGE, 0u,
                   ctl->system_stage, ctl->system_flags,
                   ctl->reset_reason, ctl->system_stage);
}

void ctl_set_stage(shared_ctrl_t *ctl, uint32_t stage) {
    ctl->system_stage = stage;
    ctl_flush(ctl);
    persist_append(ctl->boot_count, PERSIST_EVT_STAGE, 0u,
                   stage, ctl->system_flags, ctl->reset_reason, stage);
}

uint32_t ctl_next_cmd_seq(shared_ctrl_t *ctl) {
    ctl->kernel_cmd_seq++;
    ctl_flush(ctl);
    return ctl->kernel_cmd_seq;
}

void ctl_note_boot_abi(shared_ctrl_t *ctl, uint32_t hartid, uintptr_t dtb_addr) {
    ctl->boot_hartid = hartid;
    ctl->boot_dtb_addr = (uint32_t)dtb_addr;
    ctl_flush(ctl);
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
    persist_append(ctl->boot_count, PERSIST_EVT_FAULT, tag, code,
                   arg0, arg1, ctl->system_stage);
}

void ctl_trace_log(shared_ctrl_t *ctl, uint32_t source, uint32_t code,
                   uint32_t arg0, uint32_t arg1) {
    uint32_t slot = ctl->trace_log_head & (KRAKEN_TRACE_LOG_SIZE - 1u);

    ctl->trace_log[slot].source = source;
    ctl->trace_log[slot].code = code;
    ctl->trace_log[slot].arg0 = arg0;
    ctl->trace_log[slot].arg1 = arg1;
    ctl->trace_last_source = source;
    ctl->trace_last_code = code;
    ctl->trace_log_head = (slot + 1u) & (KRAKEN_TRACE_LOG_SIZE - 1u);
    if (ctl->trace_log_count < KRAKEN_TRACE_LOG_SIZE)
        ctl->trace_log_count++;
    ctl_flush(ctl);
    persist_append(ctl->boot_count, PERSIST_EVT_TRACE, source, code,
                   arg0, arg1, ctl->system_stage);
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
    ctl_trace_log(ctl, source_tag, TRACE_TRAP_PANIC,
                  packed_mcause, (uint32_t)mepc);
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

void ctl_persist_clear(void) {
    kraken_persist_log_t *log = persistent_log();

    sg2002_memset((void *)log, 0, sizeof(*log));
    log->magic = KRAKEN_PERSIST_MAGIC;
    log->version = KRAKEN_PERSIST_VERSION;
    persist_flush_header(log);
}
