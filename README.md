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

## Directories

- `project/sg2002_bootable_repo/`: main bootable bare-metal repo.
- `deploy/sg2002_deploy_pkg/`: custom deployment helper package.
- `uboot_basic/sg2002_uboot_pkg/`: simple U-Boot staging package.
- `uboot_full/sg2002_full_bundle/`: integrated U-Boot bring-up bundle.
