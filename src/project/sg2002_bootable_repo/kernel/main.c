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
    sg2002_user_led_panic_loop();
}

static void maybe_stage_worker(shared_ctrl_t *ctl) {
    (void)ctl;
#if WORKER_STAGING_ADDR != 0
    kraken_staged_image_footer_t footer;

    if (!sg2002_image_present(WORKER_LOAD_ADDR) &&
        sg2002_validate_staged_image(WORKER_STAGING_ADDR,
                                     /* max_len = payload area + footer:
                                      * sg2002_find_staged_image places the
                                      * footer at (max_len - sizeof(footer)),
                                      * which lands exactly at
                                      * WORKER_IMAGE_MAX_BYTES — the end of
                                      * the payload region. */
                                     WORKER_IMAGE_MAX_BYTES +
                                         sizeof(kraken_staged_image_footer_t),
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
    for (uint32_t spins = 0; spins < WORKER_ACK_SPINS; ++spins) {
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
    ctl_trace_log(ctl, FAULTSRC_KERNEL, TRACE_KERNEL_RESTART_WORKER,
                  reason, ctl->worker_restart_count + 1u);
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
    kraken_persist_log_t *persist = persistent_log();

    inval_dcache_range((uintptr_t)persist, (uintptr_t)persist + sizeof(*persist));
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
    if (persist->magic == KRAKEN_PERSIST_MAGIC &&
        persist->version == KRAKEN_PERSIST_VERSION) {
        console_puts(" plog=");
        console_puthex(persist->record_count);
        console_puts(" pseq=");
        console_puthex(persist->next_seq);
    }
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

static void print_trace(shared_ctrl_t *ctl) {
    uint32_t count = ctl->trace_log_count;
    uint32_t head = ctl->trace_log_head;

    if (count == 0) {
        console_puts("[trace] none\n");
        return;
    }

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t slot = (head + KRAKEN_TRACE_LOG_SIZE - count + i) &
                        (KRAKEN_TRACE_LOG_SIZE - 1u);
        const volatile kraken_trace_record_t *rec = &ctl->trace_log[slot];
        console_puts("[trace] src=");
        console_puthex(rec->source);
        console_puts(" code=");
        console_puthex(rec->code);
        console_puts(" a0=");
        console_puthex(rec->arg0);
        console_puts(" a1=");
        console_puthex(rec->arg1);
        console_puts("\n");
    }
}

static const char *persist_kind_name(uint32_t kind) {
    switch (kind) {
    case PERSIST_EVT_STAGE:
        return "stage";
    case PERSIST_EVT_TRACE:
        return "trace";
    case PERSIST_EVT_FAULT:
        return "fault";
    case PERSIST_EVT_BOOT_SUMMARY:
        return "boot";
    default:
        return "unknown";
    }
}

static void print_persist(shared_ctrl_t *ctl) {
    kraken_persist_log_t *log = persistent_log();
    (void)ctl;

    inval_dcache_range((uintptr_t)log, (uintptr_t)log + sizeof(*log));
    if (log->magic != KRAKEN_PERSIST_MAGIC ||
        log->version != KRAKEN_PERSIST_VERSION) {
        console_puts("[persist] none\n");
        return;
    }

    console_puts("[persist] count=");
    console_puthex(log->record_count);
    console_puts(" seq=");
    console_puthex(log->next_seq);
    console_puts(" prev_boot=");
    console_puthex(log->last_boot_count);
    console_puts(" prev_rst=");
    console_puthex(log->last_reset_reason);
    console_puts(" prev_stage=");
    console_puthex(log->last_stage);
    console_puts(" prev_flags=");
    console_puthex(log->last_system_flags);
    console_puts(" prev_trace=");
    console_puthex(log->last_trace_source);
    console_puts(":");
    console_puthex(log->last_trace_code);
    console_puts(" prev_fault=");
    console_puthex(log->last_fault_tag);
    console_puts(":");
    console_puthex(log->last_fault_code);
    console_puts("\n");

    for (uint32_t i = 0; i < log->record_count; ++i) {
        uint32_t slot = (log->write_head + KRAKEN_PERSIST_LOG_CAPACITY -
                         log->record_count + i) &
                        (KRAKEN_PERSIST_LOG_CAPACITY - 1u);
        const volatile kraken_persist_record_t *rec = &log->records[slot];
        console_puts("[persist] seq=");
        console_puthex(rec->seq);
        console_puts(" boot=");
        console_puthex(rec->boot_count);
        console_puts(" kind=");
        console_puts(persist_kind_name(rec->kind));
        console_puts(" src=");
        console_puthex(rec->source);
        console_puts(" code=");
        console_puthex(rec->code);
        console_puts(" a0=");
        console_puthex(rec->arg0);
        console_puts(" a1=");
        console_puthex(rec->arg1);
        console_puts(" stage=");
        console_puthex(rec->stage);
        console_puts("\n");
    }
}

static void handle_console(shared_ctrl_t *ctl) {
    /* 64 bytes including the NUL terminator; longer lines are truncated.
     * line_len tracks the number of printable bytes written (not including
     * the NUL), so line[line_len] is always the NUL sentinel. */
    static char line[64];
    static uint32_t line_len = 0;
    char buf[32];
    size_t n = usb_serial_read(buf, sizeof(buf));
    for (size_t i = 0; i < n; ++i) {
        char c = buf[i];
        if (c == '\r') continue;
        if (c == '\n') {
            line[line_len] = '\0';
            /* Compare using KRAKEN_STRLIT_LEN (sizeof - 1) so the NUL
             * terminator in the string literal is never included in the
             * compare length, and guard with an exact length check first so
             * we never read stale bytes beyond line_len. */
            if (line_len == KRAKEN_STRLIT_LEN("status") &&
                sg2002_memcmp(line, "status", KRAKEN_STRLIT_LEN("status")) == 0) {
                print_status(ctl);
            } else if (line_len == KRAKEN_STRLIT_LEN("cpu") &&
                       sg2002_memcmp(line, "cpu", KRAKEN_STRLIT_LEN("cpu")) == 0) {
                print_cpu(ctl);
            } else if (line_len == KRAKEN_STRLIT_LEN("trap") &&
                       sg2002_memcmp(line, "trap", KRAKEN_STRLIT_LEN("trap")) == 0) {
                print_trap(ctl);
            } else if (line_len == KRAKEN_STRLIT_LEN("faults") &&
                       sg2002_memcmp(line, "faults", KRAKEN_STRLIT_LEN("faults")) == 0) {
                print_faults(ctl);
            } else if (line_len == KRAKEN_STRLIT_LEN("trace") &&
                       sg2002_memcmp(line, "trace", KRAKEN_STRLIT_LEN("trace")) == 0) {
                print_trace(ctl);
            } else if (line_len == KRAKEN_STRLIT_LEN("persist") &&
                       sg2002_memcmp(line, "persist", KRAKEN_STRLIT_LEN("persist")) == 0) {
                print_persist(ctl);
            } else if (line_len == KRAKEN_STRLIT_LEN("persist-clear") &&
                       sg2002_memcmp(line, "persist-clear",
                                     KRAKEN_STRLIT_LEN("persist-clear")) == 0) {
                ctl_persist_clear();
                console_puts("[kernel] persistent log cleared\n");
            } else if (line_len == KRAKEN_STRLIT_LEN("run") &&
                       sg2002_memcmp(line, "run", KRAKEN_STRLIT_LEN("run")) == 0) {
                console_puts("[kernel] queue worker job\n");
                send_worker_cmd(ctl, CMD_RUN_JOB, 0x1234u, 0);
            } else if (line_len == KRAKEN_STRLIT_LEN("panic") &&
                       sg2002_memcmp(line, "panic", KRAKEN_STRLIT_LEN("panic")) == 0) {
                console_puts("[kernel] inject worker panic\n");
                send_worker_cmd(ctl, CMD_PANIC, 0, 0);
            } else if (line_len == KRAKEN_STRLIT_LEN("stop") &&
                       sg2002_memcmp(line, "stop", KRAKEN_STRLIT_LEN("stop")) == 0) {
                console_puts("[kernel] stop worker\n");
                send_worker_cmd(ctl, CMD_STOP, 0, 0);
            } else if (line_len != 0) {
                console_puts("[kernel] commands: status cpu trap faults trace persist persist-clear run panic stop\n");
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
    sg2002_user_led_blink(2);
    console_puts("Kraken kernel start\n");
    console_puts("[kernel] hart @ 0x"); console_puthex((uint32_t)hartid); console_puts("\n");
    console_puts("[kernel] dtb @ 0x"); console_puthex((uint32_t)dtb_addr); console_puts("\n");

    ctl_invalidate(ctl);
    if (ctl->magic != KRAKEN_MAGIC || ctl->version != KRAKEN_VERSION)
        ctl_init_defaults(ctl);
    ctl_note_boot_abi(ctl, (uint32_t)hartid, dtb_addr);
    ctl_note_riscv_boot_identity(ctl, RISCV_ID_KERNEL, (uint32_t)hartid);
    ctl_trace_log(ctl, FAULTSRC_KERNEL, TRACE_KERNEL_ENTRY,
                  (uint32_t)hartid, (uint32_t)dtb_addr);
    ctl->system_flags &= ~SYSF_BOOTLOADER_ACTIVE;
    ctl->system_flags |= SYSF_KERNEL_ACTIVE;
    ctl_set_stage(ctl, STAGE_KERNEL_ENTRY);
#if KRAKEN_ENABLE_USB_DWC2_SCAFFOLD
    if (ctl->usb_state == USB_SERIAL_OFF) usb_serial_init();
#endif

    ctl_trace_log(ctl, FAULTSRC_KERNEL, TRACE_KERNEL_BOOT_WORKER,
                  (uint32_t)WORKER_LOAD_ADDR, ctl->system_flags);
    boot_worker(ctl);
    ctl_trace_log(ctl, FAULTSRC_KERNEL, TRACE_KERNEL_WAIT_ACK,
                  ctl->worker_state, ctl->kernel_cmd_seq);
    wait_for_worker_ack(ctl);
    ctl_trace_log(ctl, FAULTSRC_KERNEL, TRACE_KERNEL_ACK_OK,
                  ctl->worker_state, ctl->worker_boot_ack);
    ensure_watchdog_started(ctl);
    ctl_trace_log(ctl, FAULTSRC_KERNEL, TRACE_KERNEL_WATCHDOG_START,
                  ctl->system_stage, ctl->kernel_pet_seq);
    ctl_set_stage(ctl, STAGE_OS_RUNNING);
    ctl_trace_log(ctl, FAULTSRC_KERNEL, TRACE_KERNEL_SUPERVISOR_LOOP,
                  ctl->worker_state, ctl->system_flags);
    sg2002_user_led_set(1);
    console_puts("[kernel] supervisor loop\n");

    uint32_t stale = 0;
    uint32_t last_worker_hb = 0;
    for (;;) {
        ctl_invalidate(ctl);
        if ((ctl->system_flags & SYSF_WATCHDOG_TIMEOUT) != 0u)
            kernel_panic(ctl,
                         ctl->reset_reason != 0u ? ctl->reset_reason
                                                 : 0x8051ffffu,
                         SYSF_WATCHDOG_TIMEOUT);
        ctl->kernel_heartbeat++;
        ctl->kernel_pet_seq++;
        ctl_flush(ctl);
        mailbox_send_8051(CMD_PET_8051, ctl->kernel_pet_seq);
        if ((ctl->kernel_heartbeat & 0x1ffffu) == 0) {
            ctl_trace_log(ctl, FAULTSRC_KERNEL, TRACE_KERNEL_HEARTBEAT,
                          ctl->kernel_heartbeat, ctl->worker_heartbeat);
            sg2002_user_led_toggle();
        }
        usb_serial_poll();
        handle_console(ctl);

        if (ctl->worker_heartbeat == last_worker_hb) stale++;
        else {
            last_worker_hb = ctl->worker_heartbeat;
            stale = 0;
        }

        if (ctl->worker_state == CORE_FAULT) {
            ctl_trace_log(ctl, FAULTSRC_KERNEL, TRACE_KERNEL_WORKER_FAULT,
                          ctl->worker_cmd, ctl->worker_cmd_ack);
            console_puts("[kernel] worker fault\n");
            restart_worker(ctl, 0xC0DE0003u);
            stale = 0;
        } else if (stale > WORKER_STALE_SPINS) {
            ctl->system_flags |= SYSF_WORKER_STALE;
            ctl_set_platform_error(ctl, PLATERR_WORKER_STALE);
            ctl_trace_log(ctl, FAULTSRC_KERNEL, TRACE_KERNEL_WORKER_STALE,
                          stale, ctl->worker_heartbeat);
            console_puts("[kernel] worker stale\n");
            restart_worker(ctl, 0xC0DE0004u);
            stale = 0;
        } else if (stale == 0) {
            ctl_clear_platform_error(ctl, PLATERR_WORKER_STALE);
        }
        delay_cycles(1000);
    }
}
