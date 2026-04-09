# SG2002 all-in-one bundle

This tarball collects a patched SG2002 bring-up tree, deployment helpers, and
U-Boot scripts.

Target board: Sipeed LicheeRV Nano W.

This repo assumes the board stays in its C906 (RISC-V) major-core boot mode so
the main Kraken payload runs on the primary C906, releases the secondary C906,
and supervises the 8051 watchdog core.

## Main improvements in this revision

- worker-core release flow now targets the public top-misc syscon model instead
  of the obviously fake `0x03002000/0x03002004` pair.
- manager/worker boot handshakes now include explicit boot-stage tracking and
  worker acknowledgements.
- package/docs flow now consistently treats `worker.bin` as preloaded unless you
  deliberately enable manager-side copying from a staging address.
- optional manager-side worker staging now uses a footer with payload size and
  CRC32 validation instead of a blind fixed-length copy.
- a USB serial subsystem and Linux-assisted CDC ACM helper flow have been added.
- the SD builder now has a Nano W RISC-V ROM-boot mode that emits a `boot.sd`
  FIT image and copies a supplied vendor `fip.bin` into the FAT partition.

## Build modes

`src/build.sh` supports two distinct SD card layouts:

- `BUILD_TARGET=staging` keeps the older FAT staging image for boards that
  already have a vendor boot chain in place.
- `BUILD_TARGET=licheerv_nano_w_riscv` builds a Nano W ROM-bootable FAT image.
  This mode requires `LICHEERV_NANO_FIP_BIN=/path/to/fip.bin` so the card
  contains both `fip.bin` and a `boot.sd` FIT that preloads the Kraken images
  into DDR before jumping to `bootloader.bin`.

The Nano W ROM-boot path now mirrors Sipeed's rootless SD image layout more
closely:

- a `16 MiB` bootable FAT32 partition starting at sector `1`
- a second Linux/ext4 partition filling the rest of the image
- vendor marker files such as `usb.dev`, `usb.ncm`, `usb.rndis`, `wifi.sta`,
  `gt9xx`, and `ver`

`src/build.sh` now assembles those images rootlessly with `mtools` and
filesystem images, so it no longer needs loop devices or sudo just to package
the SD card image.

Examples:

- `PROJECT_OUT=build-local ./src/build.sh`
- `PROJECT_OUT=build-local BUILD_TARGET=licheerv_nano_w_riscv LICHEERV_NANO_FIP_BIN=/path/to/fip.bin ./src/build.sh`

If an earlier flashing run left `src/out/` root-owned, point `OUT_DIR` at a
writable directory such as `/tmp/kraken-out`.

## Directories

- `project/sg2002_bootable_repo/`: main bootable bare-metal repo.
- `deploy/sg2002_deploy_pkg/`: custom deployment helper package.
- `uboot_basic/sg2002_uboot_pkg/`: simple U-Boot staging package.
- `uboot_full/sg2002_full_bundle/`: integrated U-Boot bring-up bundle.
