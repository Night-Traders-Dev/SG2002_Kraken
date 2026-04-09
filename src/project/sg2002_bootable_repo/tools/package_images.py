#!/usr/bin/env python3
import argparse
import binascii
import hashlib
import json
import os
import re
import shutil
import struct
from pathlib import Path

KRAKEN_STAGED_IMAGE_MAGIC = 0x4B535447
KRAKEN_STAGED_IMAGE_TAIL = 0x4B454E44
KRAKEN_STAGED_IMAGE_VERSION = 1
KRAKEN_IMAGE_WORKER = 3
STAGED_IMAGE_STRUCT = struct.Struct("<10I")
CONSTANT_NAMES = (
    "BOOTLOADER_LOAD_ADDR",
    "KERNEL_LOAD_ADDR",
    "SHARED_CTRL_ADDR",
    "WORKER_LOAD_ADDR",
    "FW8051_DDR_ADDR",
    "WORKER_STAGING_ADDR",
)


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def size(path):
    return os.path.getsize(path)


def crc32_bytes(data):
    return binascii.crc32(data) & 0xFFFFFFFF


def address_hex(value):
    return f"0x{value:08x}"


def read_constants(header_path):
    text = header_path.read_text(encoding="utf-8")
    values = {}
    for name in CONSTANT_NAMES:
        match = re.search(
            rf"^\s*#define\s+{name}\s+(0x[0-9A-Fa-f]+)(?:ull|ul|u|ll|l)?\b",
            text,
            flags=re.MULTILINE,
        )
        if not match:
            raise ValueError(f"missing {name} in {header_path}")
        values[name] = int(match.group(1), 16)
    return values


def make_staged_image(payload, image_kind, load_addr, entry_addr):
    crc = crc32_bytes(payload)
    footer = STAGED_IMAGE_STRUCT.pack(
        KRAKEN_STAGED_IMAGE_MAGIC,
        KRAKEN_STAGED_IMAGE_VERSION,
        image_kind,
        0,
        load_addr,
        entry_addr,
        len(payload),
        crc,
        crc ^ 0xFFFFFFFF,
        KRAKEN_STAGED_IMAGE_TAIL,
    )
    return payload + footer, crc


def write_load_demo(outdir, consts):
    if consts["WORKER_STAGING_ADDR"] == 0:
        staging_note = (
            "Optional manager-side staging is currently disabled because "
            "WORKER_STAGING_ADDR is 0x00000000."
        )
    else:
        staging_note = (
            "Optional manager-side staging: load worker_staged.bin to "
            f"{address_hex(consts['WORKER_STAGING_ADDR'])} and let the kernel "
            "validate and copy it."
        )

    script = f"""#!/bin/sh
set -eu
mkdir -p /mnt/data
cp bootloader.bin kernel.bin worker.bin worker_staged.bin mars_mcu_fw.bin 8051_boot_cfg.ini manifest.json /mnt/data/
echo 'Package boot flows:'
echo '1. Manual DDR staging / U-Boot script flow:'
echo '   - load bootloader.bin to {address_hex(consts["BOOTLOADER_LOAD_ADDR"])}'
echo '   - load kernel.bin to {address_hex(consts["KERNEL_LOAD_ADDR"])}'
echo '   - load worker.bin to {address_hex(consts["WORKER_LOAD_ADDR"])}'
echo '   - load mars_mcu_fw.bin to {address_hex(consts["FW8051_DDR_ADDR"])} with the vendor 8051 DDR path'
echo '   - jump to bootloader.bin'
echo '2. Nano W ROM-boot SD flow:'
echo '   - run src/build.sh BUILD_TARGET=licheerv_nano_w_riscv LICHEERV_NANO_FIP_BIN=/path/to/fip.bin'
echo '   - that builder adds fip.bin, boot.sd, boot.scr aliases, and the vendor marker files to a ROM-bootable SD image'
echo '{staging_note}'
"""
    (outdir / "load_demo.sh").write_text(script)
    os.chmod(outdir / "load_demo.sh", 0o755)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bootloader", required=True)
    parser.add_argument("--kernel", required=True)
    parser.add_argument("--worker", required=True)
    parser.add_argument("--mcu", required=True)
    parser.add_argument("--kraken-header", required=True)
    parser.add_argument("--outdir", required=True)
    args = parser.parse_args()

    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    consts = read_constants(Path(args.kraken_header))

    files = {
        "bootloader.bin": args.bootloader,
        "kernel.bin": args.kernel,
        "worker.bin": args.worker,
        "mars_mcu_fw.bin": args.mcu,
    }
    for name, src in files.items():
        shutil.copy2(src, outdir / name)

    worker_payload = Path(args.worker).read_bytes()
    worker_staged, worker_crc32 = make_staged_image(
        worker_payload,
        KRAKEN_IMAGE_WORKER,
        consts["WORKER_LOAD_ADDR"],
        consts["WORKER_LOAD_ADDR"],
    )
    (outdir / "worker_staged.bin").write_bytes(worker_staged)

    meta = {}
    for name in files:
        dst = outdir / name
        meta[name] = {"size": size(dst), "sha256": sha256(dst)}
    meta["worker_staged.bin"] = {
        "size": len(worker_staged),
        "sha256": sha256(outdir / "worker_staged.bin"),
        "payload_size": len(worker_payload),
        "payload_crc32": f"0x{worker_crc32:08x}",
        "format": "raw-binary-plus-kraken-footer",
        "load_addr": address_hex(consts["WORKER_LOAD_ADDR"]),
        "entry_addr": address_hex(consts["WORKER_LOAD_ADDR"]),
    }

    worker_staging = {
        "file": "worker_staged.bin",
        "format": "raw worker payload followed by a Kraken staged-image footer",
        "enabled": consts["WORKER_STAGING_ADDR"] != 0,
    }
    if consts["WORKER_STAGING_ADDR"] != 0:
        worker_staging["staging_addr"] = address_hex(consts["WORKER_STAGING_ADDR"])
        worker_staging["intended_use"] = (
            "load at WORKER_STAGING_ADDR when enabling manager-side worker staging"
        )
    else:
        worker_staging["intended_use"] = (
            "rebuild with a non-zero WORKER_STAGING_ADDR to enable manager-side worker staging"
        )

    manifest = {
        "target_board": "Sipeed LicheeRV Nano W",
        "bootloader_load_addr": address_hex(consts["BOOTLOADER_LOAD_ADDR"]),
        "kernel_load_addr": address_hex(consts["KERNEL_LOAD_ADDR"]),
        "shared_ctrl_addr": address_hex(consts["SHARED_CTRL_ADDR"]),
        "worker_load_addr": address_hex(consts["WORKER_LOAD_ADDR"]),
        "mcu_ddr_addr": address_hex(consts["FW8051_DDR_ADDR"]),
        "board_assumptions": {
            "major_core_boot_mode": "C906 (RISC-V)",
            "usb_port": "USB2.0 OTG Type-C",
            "notes": "the W variant adds onboard wireless peripherals but does not change the SG2002 AMP boot model used by Kraken",
        },
        "boot_modes": {
            "manual_staging": {
                "description": "load the raw binaries from U-Boot or another vendor staging environment",
                "bootloader_entry": address_hex(consts["BOOTLOADER_LOAD_ADDR"]),
                "kernel_entry": address_hex(consts["KERNEL_LOAD_ADDR"]),
                "worker_entry": address_hex(consts["WORKER_LOAD_ADDR"]),
            },
            "nano_w_rom_boot_sd": {
                "builder": "src/build.sh BUILD_TARGET=licheerv_nano_w_riscv LICHEERV_NANO_FIP_BIN=/path/to/fip.bin",
                "sd_layout": "vendor-style MBR image with a 16 MiB FAT boot partition at sector 1 and a second Linux/ext4 partition",
                "boot_partition_files": [
                    "fip.bin",
                    "boot.sd",
                    "boot.scr",
                    "boot.scr.uimg",
                    "boot_sg2002_full.scr",
                    "usb.dev",
                    "usb.ncm",
                    "usb.rndis",
                    "wifi.sta",
                    "gt9xx",
                    "ver",
                ],
            },
        },
        "worker_ctrl_method": {
            "syscon_base": "0x03000000",
            "ctrl_reg": "0x03000004",
            "ctrl_enable_mask": "0x00002000",
            "bootaddr_lo_reg": "0x03000020",
            "bootaddr_hi_reg": "0x03000024",
            "note": "the reset pulse hook is enabled by default; override KRAKEN_WORKER_RESET_* only for board experiments",
        },
        "usb_serial": {
            "mode": "cdc-acm-console-subsystem",
            "udc_name": "40e0000.cvi-usb-dev",
            "otg_base": "0x040c0000",
            "dev_base": "0x040e0000",
            "note": "the native bare-metal DWC2 backend is still a scaffold until EP0 handling and endpoint scheduling are completed; it is disabled by default unless you build with -DKRAKEN_ENABLE_USB_DWC2_SCAFFOLD=1",
        },
        "worker_staging": worker_staging,
        "files": meta,
    }
    (outdir / "manifest.json").write_text(json.dumps(manifest, indent=2))
    (outdir / "8051_boot_cfg.ini").write_text(
        "[8051_BOOT]\n"
        "firmware=mars_mcu_fw.bin\n"
        "boot_mode=ddr\n"
        f"ddr_addr={address_hex(consts['FW8051_DDR_ADDR'])}\n"
        "size_hint_kb=512\n"
    )
    write_load_demo(outdir, consts)


if __name__ == "__main__":
    main()
