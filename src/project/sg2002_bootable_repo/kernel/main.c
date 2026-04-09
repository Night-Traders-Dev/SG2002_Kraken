#include "kraken.h"

uint32_t kraken_trap_source_tag(void) {
    return FAULTSRC_KERNEL;
}

const char *kraken_trap_source_name(void) {
    return "kernel";
}

static void kernel_panic(shared_ctrl_t *ctl, uint32_t reason, uint32_t flags) {
    ctl_fault_log(ctl, FAULTSRC_KERNEL, reason, flags, ctl->system_stage);
    ctl->reset_reason = reason;
    ctl->system_flags |= flags;
    ctl_set_stage(ctl, STAGE_PANIC);
    console_puts("[kernel] panic\n");
    for (;;) cpu_relax();
}

static void maybe_stage_worker(shared_ctrl_t *ctl) {
    (void)ctl;
#if WORKER_STAGING_ADDR != 0
    kraken_staged_image_footer_t footer;

    if (!sg2002_image_present(WORKER_LOAD_ADDR) &&
        sg2002_validate_staged_image(WORKER_STAGING_ADDR,
                                     WORKER_IMAGE_MAX_BYTES + sizeof(footer),
                                     KRAKEN_IMAGE_WORKER,
                                     WORKER_LOAD_ADDR,
                                     WORKER_LOAD_ADDR,
                                     &footer)) {
        size_t copied = 0;
        console_puts("[kernel] staging worker image\n");
        sg2002_copy_image(WORKER_LOAD_ADDR, WORKER_STAGING_ADDR,
                          footer.payload_size, &copied);
        ctl->system_flags &= ~SYSF_WORKER_IMAGE_MISSING;
        ctl_flush(ctl);
        console_puts("[kernel] worker bytes=0x");
        console_puthex((uint32_t)copied);
        console_puts("\n");
    } else if (!sg2002_image_present(WORKER_LOAD_ADDR)) {
        ctl_set_platform_error(ctl, PLATERR_WORKER_STAGING_INVALID);
        console_puts("[kernel] no valid staged worker image\n");
    }
#endif
}

static void send_worker_cmd(shared_ctrl_t *ctl, uint32_t cmd, uint32_t arg0, uint32_t arg1) {
    ctl->worker_cmd = cmd;
    ctl->worker_arg0 = arg0;
    ctl->worker_arg1 = arg1;
    ctl_next_cmd_seq(ctl);
    ctl_flush(ctl);
}

static void boot_worker(shared_ctrl_t *ctl) {
    int release_status;

    ctl_set_stage(ctl, STAGE_WORKER_PREP);
    maybe_stage_worker(ctl);
    if (!sg2002_image_present(WORKER_LOAD_ADDR))
        kernel_panic(ctl, 0xC0DE0001u, SYSF_WORKER_IMAGE_MISSING);

    ctl->worker_state = CORE_BOOTING;
    ctl->worker_boot_ack = 0;
    send_worker_cmd(ctl, CMD_BOOT, (uint32_t)WORKER_LOAD_ADDR, (uint32_t)WORKER_SHARED_TOP);

    mailbox_send_worker(CMD_BOOT, (uint32_t)WORKER_LOAD_ADDR);
    ctl_set_stage(ctl, STAGE_WORKER_RELEASE);
    console_puts("[kernel] release worker core\n");
    release_status = sg2002_release_worker_core(WORKER_LOAD_ADDR);
    if (release_status != SG2002_WORKER_RELEASE_OK) {
        ctl_set_platform_error(ctl, PLATERR_WORKER_RELEASE_FAILED);
        ctl_fault_log(ctl, FAULTSRC_KERNEL, 0xC0DE0005u,
                      (uint32_t)release_status, (uint32_t)WORKER_LOAD_ADDR);
        console_puts("[kernel] worker release failed code=0x");
        console_puthex((uint32_t)release_status);
        console_puts("\n");
        kernel_panic(ctl, 0xC0DE0005u, SYSF_WORKER_RELEASE_FAIL);
    }
    ctl_clear_platform_error(ctl, PLATERR_WORKER_RELEASE_FAILED);
}

static void wait_for_worker_ack(shared_ctrl_t *ctl) {
    ctl_set_stage(ctl, STAGE_WORKER_WAIT_ACK);
    console_puts("[kernel] wait worker ack\n");
    for (uint32_t spins = 0; spins < 300000u; ++spins) {
        ctl_invalidate(ctl);
        if (ctl->worker_boot_ack == KRAKEN_MAGIC &&
            (ctl->worker_state == CORE_IDLE || ctl->worker_state == CORE_RUNNING)) {
            ctl_clear_platform_error(ctl, PLATERR_WORKER_ACK_TIMEOUT);
            console_puts("[kernel] worker online\n");
            return;
        }
        delay_cycles(64);
    }
    ctl_set_platform_error(ctl, PLATERR_WORKER_ACK_TIMEOUT);
    kernel_panic(ctl, 0xC0DE0002u, SYSF_WORKER_STALE);
}

static void restart_worker(shared_ctrl_t *ctl, uint32_t reason) {
    ctl_fault_log(ctl, FAULTSRC_KERNEL, reason,
                  ctl->worker_restart_count + 1u, ctl->worker_heartbeat);
    ctl->worker_restart_count++;
    ctl->reset_reason = reason;
    ctl->system_flags |= SYSF_WORKER_RESTARTING;
    ctl_flush(ctl);
    boot_worker(ctl);
    wait_for_worker_ack(ctl);
}

static void print_status(shared_ctrl_t *ctl) {
    console_puts("[status] stage=");
    console_puthex(ctl->system_stage);
    console_puts(" worker=");
    console_puthex(ctl->worker_state);
    console_puts(" khb=");
    console_puthex(ctl->kernel_heartbeat);
    console_puts(" whb=");
    console_puthex(ctl->worker_heartbeat);
    console_puts(" rst=");
    console_puthex(ctl->worker_restart_count);
    console_puts(" faults=");
    console_puthex(ctl->fault_log_count);
    console_puts(" traps=");
    console_puthex(ctl->trap_count);
    console_puts(" caps=");
    console_puthex(ctl->platform_caps);
    console_puts(" perr=");
    console_puthex(ctl->platform_errors);
    console_puts("\n");
}

static void print_cpu_identity(const char *label,
                               const volatile kraken_riscv_identity_t *identity) {
    console_puts("[cpu] ");
    console_puts(label);
    console_puts(" hart=");
    console_puthex(identity->hartid);
    console_puts(" mvendor=");
    console_puthex(identity->mvendorid);
    console_puts(" march=");
    console_puthex(identity->marchid);
    console_puts(" mimp=");
    console_puthex(identity->mimpid);
    console_puts(" misa=");
    console_puthex(identity->misa);
    console_puts("\n");
}

static void print_cpu(shared_ctrl_t *ctl) {
    print_cpu_identity("boot", &ctl->riscv_identity[RISCV_ID_BOOTLOADER]);
    print_cpu_identity("kernel", &ctl->riscv_identity[RISCV_ID_KERNEL]);
    print_cpu_identity("worker", &ctl->riscv_identity[RISCV_ID_WORKER]);
}

static void print_trap(shared_ctrl_t *ctl) {
    if (ctl->trap_count == 0) {
        console_puts("[trap] none\n");
        return;
    }

    console_puts("[trap] src=");
    console_puthex(ctl->trap_last_source);
    console_puts(" cause=");
    console_puthex(ctl->trap_last_cause);
    console_puts(" epc=");
    console_puthex(ctl->trap_last_epc);
    console_puts(" tval=");
    console_puthex(ctl->trap_last_tval);
    console_puts(" status=");
    console_puthex(ctl->trap_last_status);
    console_puts("\n");
}

static void print_faults(shared_ctrl_t *ctl) {
    uint32_t count = ctl->fault_log_count;
    uint32_t head = ctl->fault_log_head;

    if (count == 0) {
        console_puts("[fault] none\n");
        return;
    }

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t slot = (head + KRAKEN_FAULT_LOG_SIZE - count + i) &
                        (KRAKEN_FAULT_LOG_SIZE - 1u);
        const volatile kraken_fault_record_t *rec = &ctl->fault_log[slot];
        console_puts("[fault] tag=");
        console_puthex(rec->tag);
        console_puts(" code=");
        console_puthex(rec->code);
        console_puts(" a0=");
        console_puthex(rec->arg0);
        console_puts(" a1=");
        console_puthex(rec->arg1);
        console_puts("\n");
    }
}

static void handle_console(shared_ctrl_t *ctl) {
    static char line[64];
    static uint32_t line_len = 0;
    char buf[32];
    size_t n = usb_serial_read(buf, sizeof(buf));
    for (size_t i = 0; i < n; ++i) {
        char c = buf[i];
        if (c == '\r') continue;
        if (c == '\n') {
            line[line_len] = '\0';
            if (sg2002_memcmp(line, "status", 7) == 0) {
                print_status(ctl);
            } else if (sg2002_memcmp(line, "cpu", 4) == 0) {
                print_cpu(ctl);
            } else if (sg2002_memcmp(line, "trap", 5) == 0) {
                print_trap(ctl);
            } else if (sg2002_memcmp(line, "faults", 7) == 0) {
                print_faults(ctl);
            } else if (sg2002_memcmp(line, "run", 4) == 0) {
                console_puts("[kernel] queue worker job\n");
                send_worker_cmd(ctl, CMD_RUN_JOB, 0x1234u, 0);
            } else if (sg2002_memcmp(line, "panic", 6) == 0) {
                console_puts("[kernel] inject worker panic\n");
                send_worker_cmd(ctl, CMD_PANIC, 0, 0);
            } else if (sg2002_memcmp(line, "stop", 5) == 0) {
                console_puts("[kernel] stop worker\n");
                send_worker_cmd(ctl, CMD_STOP, 0, 0);
            } else if (line_len != 0) {
                console_puts("[kernel] commands: status cpu trap faults run panic stop\n");
            }
            line_len = 0;
            continue;
        }
        if (line_len + 1 < sizeof(line)) line[line_len++] = c;
    }
}

static void ensure_watchdog_started(shared_ctrl_t *ctl) {
    if (ctl->system_stage >= STAGE_WATCHDOG_BOOT)
        return;

    ctl_set_stage(ctl, STAGE_WATCHDOG_BOOT);
    console_puts("[kernel] starting 8051 watchdog\n");
    sg2002_boot_8051(FW8051_DDR_ADDR);
}

void kernel_main(uintptr_t hartid, uintptr_t dtb_addr) {
    shared_ctrl_t *ctl = shared_ctrl();
    console_puts("Kraken kernel start\n");
    console_puts("[kernel] hart @ 0x"); console_puthex((uint32_t)hartid); console_puts("\n");
    console_puts("[kernel] dtb @ 0x"); console_puthex((uint32_t)dtb_addr); console_puts("\n");

    ctl_invalidate(ctl);
    if (ctl->magic != KRAKEN_MAGIC || ctl->version != KRAKEN_VERSION)
        ctl_init_defaults(ctl);
    ctl_note_boot_abi(ctl, (uint32_t)hartid, dtb_addr);
    ctl_note_riscv_boot_identity(ctl, RISCV_ID_KERNEL, (uint32_t)hartid);
    ensure_watchdog_started(ctl);

    ctl->system_flags &= ~SYSF_BOOTLOADER_ACTIVE;
    ctl->system_flags |= SYSF_KERNEL_ACTIVE;
    ctl_set_stage(ctl, STAGE_KERNEL_ENTRY);
#if KRAKEN_ENABLE_USB_DWC2_SCAFFOLD
    if (ctl->usb_state == USB_SERIAL_OFF) usb_serial_init();
#endif

    boot_worker(ctl);
    wait_for_worker_ack(ctl);
    ctl_set_stage(ctl, STAGE_OS_RUNNING);
    console_puts("[kernel] supervisor loop\n");

    uint32_t stale = 0;
    uint32_t last_worker_hb = 0;
    for (;;) {
        ctl_invalidate(ctl);
        ctl->kernel_heartbeat++;
        ctl->kernel_pet_seq++;
        ctl_flush(ctl);
        mailbox_send_8051(CMD_PET_8051, ctl->kernel_pet_seq);
        usb_serial_poll();
        handle_console(ctl);

        if (ctl->worker_heartbeat == last_worker_hb) stale++;
        else {
            last_worker_hb = ctl->worker_heartbeat;
            stale = 0;
        }

        if (ctl->worker_state == CORE_FAULT) {
            console_puts("[kernel] worker fault\n");
            restart_worker(ctl, 0xC0DE0003u);
            stale = 0;
        } else if (stale > 100000u) {
            ctl->system_flags |= SYSF_WORKER_STALE;
            ctl_set_platform_error(ctl, PLATERR_WORKER_STALE);
            console_puts("[kernel] worker stale\n");
            restart_worker(ctl, 0xC0DE0004u);
            stale = 0;
        } else if (stale == 0) {
            ctl_clear_platform_error(ctl, PLATERR_WORKER_STALE);
        }
        delay_cycles(1000);
    }
}
