#include "kraken.h"

uint32_t kraken_trap_source_tag(void) {
    return FAULTSRC_BOOTLOADER;
}

const char *kraken_trap_source_name(void) {
    return "boot";
}

static void boot_panic(shared_ctrl_t *ctl, uint32_t reason, uint32_t flags) {
    ctl_fault_log(ctl, FAULTSRC_BOOTLOADER, reason, flags, ctl->system_stage);
    ctl->reset_reason = reason;
    ctl->system_flags |= flags;
    ctl_set_stage(ctl, STAGE_PANIC);
    console_puts("[boot] panic\n");
    sg2002_user_led_panic_loop();
}

static void print_previous_boot_summary(void) {
    kraken_persist_log_t *log = persistent_log();
    uint32_t source;
    uint32_t code;

    inval_dcache_range((uintptr_t)log, (uintptr_t)log + sizeof(*log));
    if (log->magic != KRAKEN_PERSIST_MAGIC ||
        log->version != KRAKEN_PERSIST_VERSION ||
        log->last_boot_count == 0u)
        return;

    source = log->last_fault_tag != 0u ? log->last_fault_tag : log->last_trace_source;
    code = log->last_fault_code != 0u ? log->last_fault_code : log->last_trace_code;

    console_puts("[boot] previous boot=");
    console_puthex(log->last_boot_count);
    console_puts(" stage=");
    console_puthex(log->last_stage);
    console_puts(" reason=");
    console_puthex(log->last_reset_reason);
    console_puts(" flags=");
    console_puthex(log->last_system_flags);
    console_puts(" src=");
    console_puthex(source);
    console_puts(" code=");
    console_puthex(code);
    console_puts("\n");
}

void bootloader_main(uintptr_t hartid, uintptr_t dtb_addr) {
    shared_ctrl_t *ctl = shared_ctrl();
    kraken_persist_log_t *persist = persistent_log();

    sg2002_user_led_blink(1);
    ctl_init_defaults(ctl);
    print_previous_boot_summary();
    sg2002_user_led_show_persist_summary(persist);
    ctl_note_boot_abi(ctl, (uint32_t)hartid, dtb_addr);
    ctl_note_riscv_boot_identity(ctl, RISCV_ID_BOOTLOADER, (uint32_t)hartid);
    ctl->system_flags |= SYSF_BOOTLOADER_ACTIVE;
    ctl_set_stage(ctl, STAGE_BOOTLOADER);
    ctl_trace_log(ctl, FAULTSRC_BOOTLOADER, TRACE_BOOT_ENTRY,
                  (uint32_t)hartid, (uint32_t)dtb_addr);
#if KRAKEN_ENABLE_USB_DWC2_SCAFFOLD
    usb_serial_init();
#endif

    console_puts("Kraken bootloader start\n");
    console_puts("[boot] hart @ 0x"); console_puthex((uint32_t)hartid); console_puts("\n");
    console_puts("[boot] dtb @ 0x"); console_puthex((uint32_t)dtb_addr); console_puts("\n");
    console_puts("[boot] kernel @ 0x"); console_puthex((uint32_t)KERNEL_LOAD_ADDR); console_puts("\n");
    console_puts("[boot] worker @ 0x"); console_puthex((uint32_t)WORKER_LOAD_ADDR); console_puts("\n");

    if (!sg2002_image_present(KERNEL_LOAD_ADDR))
        boot_panic(ctl, 0xB0070001u, SYSF_KERNEL_IMAGE_MISSING);
    if (!sg2002_image_present(WORKER_LOAD_ADDR))
        ctl->system_flags |= SYSF_WORKER_IMAGE_MISSING;
    ctl_trace_log(ctl, FAULTSRC_BOOTLOADER, TRACE_BOOT_IMAGES_READY,
                  ctl->system_flags, (uint32_t)KERNEL_LOAD_ADDR);
    ctl_flush(ctl);

    ctl_set_stage(ctl, STAGE_KERNEL_ENTRY);
    ctl_trace_log(ctl, FAULTSRC_BOOTLOADER, TRACE_BOOT_HANDOFF_KERNEL,
                  (uint32_t)KERNEL_LOAD_ADDR, ctl->system_stage);
    console_puts("[boot] handoff kernel\n");
    kraken_jump_to(KERNEL_LOAD_ADDR);
}
