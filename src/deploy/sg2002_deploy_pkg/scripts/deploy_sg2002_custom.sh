#!/bin/sh
set -eu

BOOTLOADER_BIN=${BOOTLOADER_BIN:-bootloader.bin}
KERNEL_BIN=${KERNEL_BIN:-kernel.bin}
WORKER_BIN=${WORKER_BIN:-worker.bin}
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
- place bootloader.bin at 0x80080000
- place kernel.bin at 0x80100000
- place worker.bin at 0x80180000
- clear the shared control region at 0x80170000 before first boot
- invoke 8051_up inside the staged directory if you are using the vendor 8051 loader
- jump to bootloader.bin; it will start the 8051 and hand off to kernel.bin
TXT
}

start_8051() {
    if [ "$RUN_8051_UP" != 1 ]; then
        log "skip 8051_up"
        return
    fi
    if [ -x "$STAGE_DIR/8051_up" ]; then
        log "8051_up is staged and ready in $STAGE_DIR"
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
    start_8051
    emit_next_steps
}

main "$@"
