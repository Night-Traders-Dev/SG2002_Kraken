#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR="$ROOT_DIR/project/sg2002_bootable_repo"
UBOOT_DIR="$ROOT_DIR/uboot_full/sg2002_full_bundle/uboot"
OUT_DIR="$ROOT_DIR/out"
IMG_PATH="$OUT_DIR/sg2002_sdcard.img"
IMG_SIZE_MIB=${IMG_SIZE_MIB:-128}
PART_START_SECTOR=${PART_START_SECTOR:-2048}
MOUNT_DIR=${MOUNT_DIR:-/mnt/sg2002_sdcard}

CROSS=${CROSS:-riscv64-unknown-elf-}
MKIMAGE=${MKIMAGE:-mkimage}
PYTHON=${PYTHON:-python3}
SUDO=${SUDO:-}
MAKE_ARGS=()

if [[ ${EUID:-$(id -u)} -ne 0 ]]; then
  SUDO=sudo
fi

log() {
  printf '[sg2002-build] %s\n' "$*"
}

die() {
  printf '[sg2002-build] ERROR: %s\n' "$*" >&2
  exit 1
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

cleanup() {
  set +e
  if mountpoint -q "$MOUNT_DIR" 2>/dev/null; then
    log "unmounting FAT partition"
    $SUDO umount "$MOUNT_DIR"
  fi
  if [[ -n "${LOOPDEV:-}" ]]; then
    log "detaching loop device"
    $SUDO losetup -d "$LOOPDEV"
  fi
}
trap cleanup EXIT

need_cmd make
need_cmd "$PYTHON"
need_cmd "$MKIMAGE"
need_cmd sfdisk
need_cmd losetup
need_cmd mkfs.vfat
need_cmd mount
need_cmd umount
need_cmd dd
need_cmd sync
need_cmd "${CROSS}gcc"
need_cmd "${CROSS}objcopy"
need_cmd "${CROSS}objdump"
need_cmd "${CROSS}size"

mkdir -p "$OUT_DIR"
mkdir -p "$MOUNT_DIR"

log "building project in $PROJECT_DIR"
make -C "$PROJECT_DIR" CROSS="$CROSS" PYTHON="$PYTHON" "${MAKE_ARGS[@]}"

log "building U-Boot script image"
(
  cd "$UBOOT_DIR"
  ./make_bootscr.sh boot_sg2002_full.txt boot_sg2002_full.scr
)

log "creating raw disk image $IMG_PATH (${IMG_SIZE_MIB} MiB)"
rm -f "$IMG_PATH"
dd if=/dev/zero of="$IMG_PATH" bs=1M count="$IMG_SIZE_MIB" status=none

log "writing MBR partition table"
cat <<PARTITION_TABLE | sfdisk "$IMG_PATH"
label: dos
unit: sectors

${IMG_PATH}1 : start=${PART_START_SECTOR}, type=c, bootable
PARTITION_TABLE

log "attaching loop device with partition scan"
LOOPDEV=$($SUDO losetup --find --show --partscan "$IMG_PATH")
PARTDEV="${LOOPDEV}p1"
[[ -b "$PARTDEV" ]] || die "partition device not found: $PARTDEV"

log "formatting FAT32 partition $PARTDEV"
$SUDO mkfs.vfat -F 32 "$PARTDEV" >/dev/null

log "mounting FAT partition $PARTDEV"
$SUDO mount "$PARTDEV" "$MOUNT_DIR"

log "copying files into FAT partition"
$SUDO cp -f \
  "$PROJECT_DIR/build/package/bootloader.bin" \
  "$PROJECT_DIR/build/package/kernel.bin" \
  "$PROJECT_DIR/build/package/worker.bin" \
  "$PROJECT_DIR/build/package/mars_mcu_fw.bin" \
  "$PROJECT_DIR/build/package/manifest.json" \
  "$PROJECT_DIR/build/package/8051_boot_cfg.ini" \
  "$PROJECT_DIR/build/package/load_demo.sh" \
  "$UBOOT_DIR/boot_sg2002_full.scr" \
  "$MOUNT_DIR/"

if [[ -f "$ROOT_DIR/README.md" ]]; then
  $SUDO cp -f "$ROOT_DIR/README.md" "$MOUNT_DIR/README.txt"
fi

sync

log "done; outputs in $OUT_DIR"
