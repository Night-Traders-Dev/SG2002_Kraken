# Roadmap

## Phase 1 — current repo
- staged bootloader / kernel / worker / 8051 layout
- shared-memory ABI
- worker release flow
- watchdog heartbeat model
- USB console subsystem contract

## Phase 2 — next hardware work
- board validation for the optional C906 cache maintenance backend
- actual secondary-core reset deassert hook if required by this board
- real 8051 watchdog/reset register programming
- loader-side image headers and checksum validation

## Recent RISC-V bring-up work
- machine-mode trap vector installation for bootloader, kernel, and worker
- shared-memory trap telemetry for cause, EPC, trap value, and status snapshots
- shared-memory capture of `misa`, `mvendorid`, `marchid`, `mimpid`, and `mhartid`
- secondary C906 release path readback for boot-address and enable-bit latching

## Phase 3 — kernel growth
- timer interrupts instead of pure spin delays
- scheduler and task model on the main core
- richer worker RPC / mailbox ABI
- richer crash records beyond the shared-memory fault log ring
- storage and filesystem bring-up

## Phase 4 — USB native device stack
- SG2002 USB device init
- EP0 control transfer support
- CDC ACM data/notification endpoints
- ring to endpoint bridge
- interactive shell over USB ACM
