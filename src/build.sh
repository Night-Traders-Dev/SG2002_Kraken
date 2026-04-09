#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR="$ROOT_DIR/project/sg2002_bootable_repo"
UBOOT_DIR="$ROOT_DIR/uboot_full/sg2002_full_bundle/uboot"
OUT_DIR=${OUT_DIR:-$ROOT_DIR/out}
PROJECT_OUT=${PROJECT_OUT:-build}
PACKAGE_DIR="$PROJECT_DIR/$PROJECT_OUT/package"
BUILD_TARGET=${BUILD_TARGET:-staging}
LICHEERV_NANO_FIP_BIN=${LICHEERV_NANO_FIP_BIN:-}
LICHEERV_NANO_BOOT_LOGO_JPEG=${LICHEERV_NANO_BOOT_LOGO_JPEG:-}
IMG_PATH=${IMG_PATH:-$OUT_DIR/sg2002_sdcard.img}
IMG_SIZE_MIB=${IMG_SIZE_MIB:-128}
PART_START_SECTOR=${PART_START_SECTOR:-}
BOOT_PART_SIZE_SECTORS=${BOOT_PART_SIZE_SECTORS:-}
BOOT_VOLUME_LABEL=${BOOT_VOLUME_LABEL:-}
ROOTFS_VOLUME_LABEL=${ROOTFS_VOLUME_LABEL:-rootfs}

CROSS=${CROSS:-riscv64-unknown-elf-}
MKIMAGE=${MKIMAGE:-mkimage}
PYTHON=${PYTHON:-python3}
MAKE_ARGS=()
BOOT_SD_PATH=
BOOTSCRIPT_PATH="$OUT_DIR/boot_sg2002_full.scr"

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

need_cmd make
need_cmd "$PYTHON"
need_cmd "$MKIMAGE"
need_cmd sfdisk
need_cmd mkfs.vfat
need_cmd mkfs.ext4
need_cmd mformat
need_cmd mcopy
need_cmd dd
need_cmd sync
need_cmd truncate
need_cmd "${CROSS}gcc"
need_cmd "${CROSS}objcopy"
need_cmd "${CROSS}objdump"
need_cmd "${CROSS}size"

case "$BUILD_TARGET" in
  staging|licheerv_nano_w_riscv)
    ;;
  *)
    die "unsupported BUILD_TARGET=$BUILD_TARGET (expected staging or licheerv_nano_w_riscv)"
    ;;
esac

if [[ "$BUILD_TARGET" == "licheerv_nano_w_riscv" ]]; then
  [[ -n "$LICHEERV_NANO_FIP_BIN" ]] || die "BUILD_TARGET=licheerv_nano_w_riscv requires LICHEERV_NANO_FIP_BIN=/path/to/fip.bin"
  [[ -f "$LICHEERV_NANO_FIP_BIN" ]] || die "LICHEERV_NANO_FIP_BIN does not exist: $LICHEERV_NANO_FIP_BIN"
  if [[ -n "$LICHEERV_NANO_BOOT_LOGO_JPEG" ]]; then
    [[ -f "$LICHEERV_NANO_BOOT_LOGO_JPEG" ]] || die "LICHEERV_NANO_BOOT_LOGO_JPEG does not exist: $LICHEERV_NANO_BOOT_LOGO_JPEG"
  fi
fi

if [[ "$BUILD_TARGET" == "licheerv_nano_w_riscv" ]]; then
  PART_START_SECTOR=${PART_START_SECTOR:-1}
  BOOT_PART_SIZE_SECTORS=${BOOT_PART_SIZE_SECTORS:-32768}
  BOOT_VOLUME_LABEL=${BOOT_VOLUME_LABEL:-boot}
else
  PART_START_SECTOR=${PART_START_SECTOR:-2048}
  BOOT_VOLUME_LABEL=${BOOT_VOLUME_LABEL:-BOOT}
fi

mkdir -p "$OUT_DIR"

git_rev() {
  if git -C "$ROOT_DIR" rev-parse --short=7 HEAD >/dev/null 2>&1; then
    git -C "$ROOT_DIR" rev-parse --short=7 HEAD
  else
    printf 'unknown'
  fi
}

nano_version_file() {
  printf '%s-%s.img\n' "$(date +%F-%H-%M)" "$(git_rev)"
}

log "building project in $PROJECT_DIR"
make -C "$PROJECT_DIR" OUT="$PROJECT_OUT" CROSS="$CROSS" PYTHON="$PYTHON" "${MAKE_ARGS[@]}"

log "building U-Boot script image"
(
  cd "$UBOOT_DIR"
  ./make_bootscr.sh boot_sg2002_full.txt "$BOOTSCRIPT_PATH"
)

if [[ "$BUILD_TARGET" == "licheerv_nano_w_riscv" ]]; then
  BOOT_SD_PATH="$OUT_DIR/boot.sd"
  log "building LicheeRV Nano W boot.sd FIT payload"
  "$PYTHON" "$ROOT_DIR/tools/make_licheerv_nano_boot_sd.py" \
    --mkimage "$MKIMAGE" \
    --package-dir "$PACKAGE_DIR" \
    --kraken-header "$PROJECT_DIR/include/kraken.h" \
    --out "$BOOT_SD_PATH" \
    --its-out "$OUT_DIR/boot.sd.its" \
    --config config-sg2002_licheervnano_sd
fi

log "creating raw disk image $IMG_PATH (${IMG_SIZE_MIB} MiB)"
rm -f "$IMG_PATH"
dd if=/dev/zero of="$IMG_PATH" bs=1M count="$IMG_SIZE_MIB" status=none

TOTAL_SECTORS=$((IMG_SIZE_MIB * 2048))
if (( PART_START_SECTOR <= 0 || PART_START_SECTOR >= TOTAL_SECTORS )); then
  die "invalid PART_START_SECTOR=$PART_START_SECTOR for IMG_SIZE_MIB=$IMG_SIZE_MIB"
fi

log "writing MBR partition table"
if [[ "$BUILD_TARGET" == "licheerv_nano_w_riscv" ]]; then
  ROOTFS_START_SECTOR=$((PART_START_SECTOR + BOOT_PART_SIZE_SECTORS))
  if (( ROOTFS_START_SECTOR >= TOTAL_SECTORS )); then
    die "BOOT_PART_SIZE_SECTORS=$BOOT_PART_SIZE_SECTORS leaves no room for a rootfs partition"
  fi
  cat <<PARTITION_TABLE | sfdisk "$IMG_PATH"
label: dos
unit: sectors

${IMG_PATH}1 : start=${PART_START_SECTOR}, size=${BOOT_PART_SIZE_SECTORS}, type=c, bootable
${IMG_PATH}2 : start=${ROOTFS_START_SECTOR}, type=83
PARTITION_TABLE
else
  BOOT_PART_SIZE_SECTORS=$((TOTAL_SECTORS - PART_START_SECTOR))
  cat <<PARTITION_TABLE | sfdisk "$IMG_PATH"
label: dos
unit: sectors

${IMG_PATH}1 : start=${PART_START_SECTOR}, type=c, bootable
PARTITION_TABLE
fi

BOOT_INPUT_DIR="$OUT_DIR/bootfs_input"
BOOTFS_IMG="$OUT_DIR/boot.vfat"
rm -rf "$BOOT_INPUT_DIR"
mkdir -p "$BOOT_INPUT_DIR"

cp -f \
  "$PACKAGE_DIR/bootloader.bin" \
  "$PACKAGE_DIR/kernel.bin" \
  "$PACKAGE_DIR/worker.bin" \
  "$PACKAGE_DIR/mars_mcu_fw.bin" \
  "$PACKAGE_DIR/manifest.json" \
  "$PACKAGE_DIR/8051_boot_cfg.ini" \
  "$PACKAGE_DIR/load_demo.sh" \
  "$BOOT_INPUT_DIR/"

cp -f "$BOOTSCRIPT_PATH" "$BOOT_INPUT_DIR/boot_sg2002_full.scr"
cp -f "$BOOTSCRIPT_PATH" "$BOOT_INPUT_DIR/boot.scr"
cp -f "$BOOTSCRIPT_PATH" "$BOOT_INPUT_DIR/boot.scr.uimg"

if [[ "$BUILD_TARGET" == "licheerv_nano_w_riscv" ]]; then
  cp -f "$BOOT_SD_PATH" "$BOOT_INPUT_DIR/boot.sd"
  cp -f "$LICHEERV_NANO_FIP_BIN" "$BOOT_INPUT_DIR/fip.bin"
  : > "$BOOT_INPUT_DIR/usb.dev"
  : > "$BOOT_INPUT_DIR/usb.ncm"
  : > "$BOOT_INPUT_DIR/usb.rndis"
  : > "$BOOT_INPUT_DIR/wifi.sta"
  : > "$BOOT_INPUT_DIR/gt9xx"
  nano_version_file > "$BOOT_INPUT_DIR/ver"
  if [[ -n "$LICHEERV_NANO_BOOT_LOGO_JPEG" ]]; then
    cp -f "$LICHEERV_NANO_BOOT_LOGO_JPEG" "$BOOT_INPUT_DIR/logo.jpeg"
  fi
fi

if [[ -f "$ROOT_DIR/README.md" ]]; then
  cp -f "$ROOT_DIR/README.md" "$BOOT_INPUT_DIR/README.txt"
fi

log "creating FAT boot filesystem image"
rm -f "$BOOTFS_IMG"
truncate -s "$((BOOT_PART_SIZE_SECTORS * 512))" "$BOOTFS_IMG"
mformat -F -i "$BOOTFS_IMG" -v "$BOOT_VOLUME_LABEL" ::
for file in "$BOOT_INPUT_DIR"/*; do
  [[ -e "$file" ]] || continue
  mcopy -i "$BOOTFS_IMG" -o "$file" ::
done

log "embedding FAT boot filesystem into raw image"
dd if="$BOOTFS_IMG" of="$IMG_PATH" bs=512 seek="$PART_START_SECTOR" conv=notrunc status=none

if [[ "$BUILD_TARGET" == "licheerv_nano_w_riscv" ]]; then
  ROOTFS_PART_SIZE_SECTORS=$((TOTAL_SECTORS - ROOTFS_START_SECTOR))
  ROOTFS_INPUT_DIR="$OUT_DIR/rootfs_input"
  ROOTFS_IMG="$OUT_DIR/rootfs.sd"
  rm -rf "$ROOTFS_INPUT_DIR"
  mkdir -p "$ROOTFS_INPUT_DIR"
  cat > "$ROOTFS_INPUT_DIR/README.kraken-rootfs.txt" <<'ROOTFS_NOTE'
This ext4 partition is a Kraken placeholder that preserves the vendor-style
LicheeRV Nano W SD layout. Kraken boots entirely from the FAT boot partition
via fip.bin + boot.sd and does not currently require a Linux rootfs.
ROOTFS_NOTE
  rm -f "$ROOTFS_IMG"
  truncate -s "$((ROOTFS_PART_SIZE_SECTORS * 512))" "$ROOTFS_IMG"
  log "creating ext4 rootfs placeholder image"
  mkfs.ext4 -F -L "$ROOTFS_VOLUME_LABEL" -d "$ROOTFS_INPUT_DIR" "$ROOTFS_IMG" >/dev/null
  log "embedding ext4 rootfs placeholder into raw image"
  dd if="$ROOTFS_IMG" of="$IMG_PATH" bs=512 seek="$ROOTFS_START_SECTOR" conv=notrunc status=none
fi

sync

log "done; outputs in $OUT_DIR"
