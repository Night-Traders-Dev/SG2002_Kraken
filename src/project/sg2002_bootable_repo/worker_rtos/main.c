#include "kraken.h"

uint32_t kraken_trap_source_tag(void) {
    return FAULTSRC_WORKER;
}

const char *kraken_trap_source_name(void) {
    return "worker";
}

static void run_demo_job(shared_ctrl_t *ctl) {
    ctl->worker_state = CORE_RUNNING;
    ctl_flush(ctl);
    for (uint32_t i = 0; i < 50000u; ++i) {
        ctl_invalidate(ctl);
        if (ctl->worker_cmd == CMD_CANCEL || ctl->worker_cmd == CMD_STOP) {
            ctl->worker_state = CORE_IDLE;
            ctl->worker_cmd_ack = ctl->kernel_cmd_seq;
            ctl->worker_cmd = CMD_NONE;
            ctl_flush(ctl);
            return;
        }
        ctl->worker_result = i ^ ctl->worker_arg0;
        ctl->worker_heartbeat++;
        ctl->worker_cmd_ack = ctl->kernel_cmd_seq;
        ctl_flush(ctl);
        delay_cycles(32);
    }
    ctl->worker_state = CORE_IDLE;
    ctl->worker_cmd_ack = ctl->kernel_cmd_seq;
    ctl->worker_cmd = CMD_NONE;
    ctl_flush(ctl);
}

void worker_main(void) {
    shared_ctrl_t *ctl = shared_ctrl();
    console_puts("Kraken worker start\n");
    ctl->worker_boot_ack = KRAKEN_MAGIC;
    ctl->worker_state = CORE_IDLE;
    ctl->worker_cmd_ack = ctl->kernel_cmd_seq;
    ctl_flush(ctl);

    for (;;) {
        ctl_invalidate(ctl);
        ctl->worker_heartbeat++;
        switch (ctl->worker_cmd) {
        case CMD_NONE:
            ctl_flush(ctl);
            delay_cycles(256);
            break;
        case CMD_BOOT:
            ctl->worker_state = CORE_IDLE;
            ctl->worker_cmd_ack = ctl->kernel_cmd_seq;
            ctl->worker_cmd = CMD_NONE;
            ctl_flush(ctl);
            break;
        case CMD_RUN_JOB:
            run_demo_job(ctl);
            break;
        case CMD_PANIC:
            ctl_fault_log(ctl, FAULTSRC_WORKER, 0xD00D0001u,
                          ctl->worker_cmd, ctl->kernel_cmd_seq);
            ctl->worker_state = CORE_FAULT;
            ctl->worker_cmd_ack = ctl->kernel_cmd_seq;
            ctl_flush(ctl);
            break;
        case CMD_STOP:
            ctl->worker_state = CORE_OFFLINE;
            ctl->worker_cmd_ack = ctl->kernel_cmd_seq;
            ctl_flush(ctl);
            for (;;) cpu_relax();
        default:
            ctl_fault_log(ctl, FAULTSRC_WORKER, 0xD00D0002u,
                          ctl->worker_cmd, ctl->kernel_cmd_seq);
            ctl->worker_state = CORE_FAULT;
            ctl->worker_cmd_ack = ctl->kernel_cmd_seq;
            ctl_flush(ctl);
            break;
        }
    }
}
