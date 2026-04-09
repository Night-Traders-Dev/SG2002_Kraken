# Architecture

## Roles

### bootloader/
Small machine-mode handoff layer that owns only the earliest post-U-Boot work:

- initialize shared control state
- sanity-check images already placed in DDR
- start the 8051 watchdog image
- jump to the main kernel image

### kernel/
Primary system controller on the main C906:

- owns the platform state machine
- boots/releases the worker core
- supervises worker health and restarts it on fault/stall
- exports a console over UART0 plus the shared USB serial rings

### worker_rtos/
Secondary C906 service processor:

- acknowledges boot
- runs jobs on command
- reports liveness into shared memory
- can be restarted independently of the main kernel

### watch8051/
Low-power hardware supervisor:

- watches kernel pet sequence
- watches worker heartbeat when the worker is running
- records a reset reason before escalation

## Shared control ABI

The shared page begins with a fixed watchdog-friendly prefix:

- `0x20`: kernel heartbeat
- `0x24`: worker heartbeat
- `0x28`: worker state
- `0x30`: kernel pet sequence
- `0x34`: watchdog last kernel sequence
- `0x38`: watchdog last worker sequence
- `0x3c`: watchdog pet count

The remainder of the page holds command/ack fields and USB serial rings.

## USB serial subsystem

The repo includes a stable USB console contract now:

- TX/RX rings in shared memory
- `usb_serial_*()` API for the bootloader/kernel/worker
- Linux-assisted ACM gadget helper scripts in the deployment bundle
- a bare-metal CDC ACM path that only queues TX data while the host is connected and flushes stale TX bytes when DTR drops

The native bare-metal UDC backend is intentionally left isolated so the board-specific endpoint engine can be added later without changing the kernel/worker ABI. EP0 handling, endpoint scheduling, and SG2002-specific interrupt routing are still unfinished.
