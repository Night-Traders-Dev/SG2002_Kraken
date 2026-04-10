#!/bin/sh
set -eu

BOOTLOADER_BIN=${BOOTLOADER_BIN:-bootloader.bin}
KERNEL_BIN=${KERNEL_BIN:-kernel.bin}
WORKER_BIN=${WORKER_BIN:-worker.bin}
WORKER_STAGED_BIN=${WORKER_STAGED_BIN:-worker_staged.bin}
MCU_BIN=${MCU_BIN:-mars_mcu_fw.bin}
BOOT_CFG=${BOOT_CFG:-8051_boot_cfg.ini}
STAGE_DIR=${STAGE_DIR:-./stage}
RUN_8051_UP=${RUN_8051_UP:-1}
DRY_RUN=${DRY_RUN:-0}

log() {
    printf '[deploy] %s\n' "$*"
}

run() {
    if [ "$DRY_RUN" = 1 ]; then
        printf '[dry-run] %s\n' "$*"
    else
        sh -c "$*"
    fi
}

need_file() {
    [ -f "$1" ] || { echo "missing file: $1" >&2; exit 1; }
}

stage_files() {
    log "staging files in $STAGE_DIR"
    run "mkdir -p '$STAGE_DIR'"
    run "cp '$BOOTLOADER_BIN' '$KERNEL_BIN' '$WORKER_BIN' '$MCU_BIN' '$BOOT_CFG' '$STAGE_DIR/'"
    if [ -f "$WORKER_STAGED_BIN" ]; then
        run "cp '$WORKER_STAGED_BIN' '$STAGE_DIR/'"
    fi
    if [ -f ./8051_up ]; then
        run "cp ./8051_up '$STAGE_DIR/'"
        run "chmod +x '$STAGE_DIR/8051_up'"
    fi
    if [ -f ./linux_enable_usb_acm.sh ]; then
        run "cp ./linux_enable_usb_acm.sh ./linux_disable_usb_gadget.sh '$STAGE_DIR/'"
        run "chmod +x '$STAGE_DIR/linux_enable_usb_acm.sh' '$STAGE_DIR/linux_disable_usb_gadget.sh'"
    fi
}

emit_next_steps() {
    cat <<TXT
Next manual integration steps for your custom loader:
- place bootloader.bin at 0x80200000
- place kernel.bin at 0x80280000
- place worker.bin at 0x80380000
- optionally place worker_staged.bin at WORKER_STAGING_ADDR if you enable manager-side worker staging
- clear the shared control region at 0x80300000 before first boot
- keep 8051_up staged only if you want to compare against the vendor 8051 loader
- jump to bootloader.bin; kernel.bin will later map and start the 8051 watchdog after worker bring-up
TXT
}

note_vendor_8051_loader() {
    if [ "$RUN_8051_UP" != 1 ]; then
        log "skip 8051_up"
        return
    fi
    if [ -x "$STAGE_DIR/8051_up" ]; then
        log "8051_up is staged in $STAGE_DIR for optional vendor-path comparisons"
    else
        log "8051_up not present; stage it manually if needed"
    fi
}

main() {
    need_file "$BOOTLOADER_BIN"
    need_file "$KERNEL_BIN"
    need_file "$WORKER_BIN"
    need_file "$MCU_BIN"
    need_file "$BOOT_CFG"
    stage_files
    note_vendor_8051_loader
    emit_next_steps
}

main "$@"
