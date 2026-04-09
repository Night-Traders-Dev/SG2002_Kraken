# SG2002 all-in-one bundle

This tarball collects a patched SG2002 bring-up tree, deployment helpers, and
U-Boot scripts.

## Main improvements in this revision

- worker-core release flow now targets the public top-misc syscon model instead
  of the obviously fake `0x03002000/0x03002004` pair.
- manager/worker boot handshakes now include explicit boot-stage tracking and
  worker acknowledgements.
- package/docs flow now consistently treats `worker.bin` as preloaded unless you
  deliberately enable manager-side copying from a staging address.
- a USB serial subsystem and Linux-assisted CDC ACM helper flow have been added.

## Directories

- `project/sg2002_bootable_repo/`: main bootable bare-metal repo.
- `deploy/sg2002_deploy_pkg/`: custom deployment helper package.
- `uboot_basic/sg2002_uboot_pkg/`: simple U-Boot staging package.
- `uboot_full/sg2002_full_bundle/`: integrated U-Boot bring-up bundle.
