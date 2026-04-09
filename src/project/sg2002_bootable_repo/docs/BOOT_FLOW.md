# Staged boot flow

## DDR layout

- `0x80080000` — `bootloader.bin`
- `0x80100000` — `kernel.bin`
- `0x80170000` — shared control page
- `0x80180000` — `worker.bin`
- `0x83f80000` — `mars_mcu_fw.bin`

## Sequence

### 1. U-Boot or another staging loader
Loads all four images and clears the shared control page.

### 2. Bootloader
Initializes the control page, enables USB-console state, starts the 8051,
and jumps to the kernel.

### 3. Kernel
Checks worker image availability, sends a mailbox boot command, writes the
secondary-core boot address, asserts the public enable bit, waits for boot ACK,
and enters supervision.

### 4. Worker
Acknowledges boot, services commands, and updates heartbeats.

### 5. 8051
Monitors kernel pet sequence and worker heartbeat and escalates reset on timeout.
