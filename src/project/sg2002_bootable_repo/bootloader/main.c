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
    for (;;) cpu_relax();
}

void bootloader_main(uintptr_t hartid, uintptr_t dtb_addr) {
    shared_ctrl_t *ctl = shared_ctrl();

    ctl_init_defaults(ctl);
    ctl_note_boot_abi(ctl, (uint32_t)hartid, dtb_addr);
    ctl_note_riscv_boot_identity(ctl, RISCV_ID_BOOTLOADER, (uint32_t)hartid);
    ctl->system_flags |= SYSF_BOOTLOADER_ACTIVE;
    ctl_set_stage(ctl, STAGE_BOOTLOADER);
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
    ctl_flush(ctl);

    ctl_set_stage(ctl, STAGE_WATCHDOG_BOOT);
    console_puts("[boot] starting 8051 watchdog\n");
    sg2002_boot_8051(FW8051_DDR_ADDR);

    ctl_set_stage(ctl, STAGE_KERNEL_ENTRY);
    console_puts("[boot] handoff kernel\n");
    kraken_jump_to(KERNEL_LOAD_ADDR);
}
