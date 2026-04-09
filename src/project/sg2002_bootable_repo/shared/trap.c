#include "kraken.h"

#define KRAKEN_TRAP_REASON 0x54524150u

static const char *trap_cause_name(uint64_t mcause) {
    uint64_t code = mcause & ((1ull << 63) - 1ull);
    if ((mcause >> 63) != 0) {
        switch (code) {
        case 1: return "supervisor software interrupt";
        case 3: return "machine software interrupt";
        case 5: return "supervisor timer interrupt";
        case 7: return "machine timer interrupt";
        case 9: return "supervisor external interrupt";
        case 11: return "machine external interrupt";
        default: return "interrupt";
        }
    }

    switch (code) {
    case 0: return "instruction address misaligned";
    case 1: return "instruction access fault";
    case 2: return "illegal instruction";
    case 3: return "breakpoint";
    case 4: return "load address misaligned";
    case 5: return "load access fault";
    case 6: return "store address misaligned";
    case 7: return "store access fault";
    case 8: return "environment call from U-mode";
    case 9: return "environment call from S-mode";
    case 11: return "environment call from M-mode";
    case 12: return "instruction page fault";
    case 13: return "load page fault";
    case 15: return "store page fault";
    default: return "exception";
    }
}

void kraken_trap_panic(uint64_t mcause, uint64_t mepc,
                       uint64_t mtval, uint64_t mstatus) {
    shared_ctrl_t *ctl = shared_ctrl();
    uint32_t source = kraken_trap_source_tag();

    uart_puts("[trap:");
    uart_puts(kraken_trap_source_name());
    uart_puts("] ");
    uart_puts(trap_cause_name(mcause));
    uart_puts("\n");
    uart_puts("[trap] mcause=0x");
    uart_puthex64(mcause);
    uart_puts(" mepc=0x");
    uart_puthex64(mepc);
    uart_puts(" mtval=0x");
    uart_puthex64(mtval);
    uart_puts(" mstatus=0x");
    uart_puthex64(mstatus);
    uart_puts("\n");

    if (ctl->magic != KRAKEN_MAGIC || ctl->version != KRAKEN_VERSION)
        ctl_init_defaults(ctl);

    ctl_note_trap(ctl, source, mcause, mepc, mtval, mstatus);
    ctl->reset_reason = KRAKEN_TRAP_REASON;
    ctl->system_flags |= SYSF_RISCV_TRAP;
    if (source == FAULTSRC_WORKER)
        ctl->worker_state = CORE_FAULT;
    ctl_set_stage(ctl, STAGE_PANIC);

    sg2002_user_led_panic_loop();
}
