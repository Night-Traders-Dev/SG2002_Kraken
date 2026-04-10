# Roadmap

## Phase 1 — current repo
- staged bootloader / kernel / worker / 8051 layout
- shared-memory ABI
- worker release flow
- watchdog heartbeat model
- USB console subsystem contract

## Phase 2 — next hardware work
- board validation for the optional C906 cache maintenance backend
- board validation for the SG200X secondary-core reset hook on Nano W hardware
- real 8051 watchdog/reset register programming
- loader-side image headers and checksum validation

## Recent RISC-V bring-up work
- machine-mode trap vector installation for bootloader, kernel, and worker
- shared-memory trap telemetry for cause, EPC, trap value, and status snapshots
- shared-memory capture of `misa`, `mvendorid`, `marchid`, `mimpid`, and `mhartid`
- secondary C906 release path readback for boot-address and enable-bit latching
- SG200X `CPUSYS2` reset pulse during worker release
- reset-persistent DDR log ring for boot, stage, trace, and fault breadcrumbs

## Phase 3 — kernel growth
- timer interrupts instead of pure spin delays
- scheduler and task model on the main core
- richer worker RPC / mailbox ABI
- richer crash records beyond the shared-memory fault log ring
- storage and filesystem bring-up for true SD/flash-backed persistent logs

## Phase 4 — USB native device stack
- finish SG2002 USB device init with the final TRM-backed clock/reset/PHY sequence
- EP0 control transfer support
- CDC ACM data/notification endpoints
- ring to endpoint bridge
- correct PLIC interrupt hookup for the USB controller
- interactive shell over USB ACM

## Recent USB reliability work
- USB console TX now only queues while the host CDC ACM port is connected
- stale USB TX bytes are dropped when the host deasserts DTR to avoid reconnect floods
- device descriptor `bcdUSB` now advertises 0x0210 to match CDC IAD usage
- Nano W vendor-DTB facts are now modeled in the scaffold: DWC2 MMIO `0x04340000`, companion USB syscon `0x03006000`, and `PLIC` source `30`
- TinyUSB's DWC2 task now has a minimal `PLIC` claim/complete polling path so it can drain pending USB IRQs without full trap-return support yet
