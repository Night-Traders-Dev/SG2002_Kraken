#!/usr/bin/env python3
import argparse, json, os, shutil, hashlib
from pathlib import Path

def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            h.update(chunk)
    return h.hexdigest()

def size(path):
    return os.path.getsize(path)

p = argparse.ArgumentParser()
p.add_argument('--bootloader', required=True)
p.add_argument('--kernel', required=True)
p.add_argument('--worker', required=True)
p.add_argument('--mcu', required=True)
p.add_argument('--outdir', required=True)
a = p.parse_args()

outdir = Path(a.outdir)
outdir.mkdir(parents=True, exist_ok=True)
files = {
    'bootloader.bin': a.bootloader,
    'kernel.bin': a.kernel,
    'worker.bin': a.worker,
    'mars_mcu_fw.bin': a.mcu,
}
for name, src in files.items():
    shutil.copy2(src, outdir / name)

meta = {}
for name in files:
    dst = outdir / name
    meta[name] = {'size': size(dst), 'sha256': sha256(dst)}

manifest = {
    'bootloader_load_addr': '0x80080000',
    'kernel_load_addr': '0x80100000',
    'shared_ctrl_addr': '0x80170000',
    'worker_load_addr': '0x80180000',
    'mcu_ddr_addr': '0x83F80000',
    'worker_ctrl_method': {
        'syscon_base': '0x03000000',
        'ctrl_reg': '0x03000004',
        'ctrl_enable_mask': '0x00002000',
        'bootaddr_lo_reg': '0x03000020',
        'bootaddr_hi_reg': '0x03000024',
        'note': 'reset deassert remains board-specific and can be added through a platform hook',
    },
    'usb_serial': {
        'mode': 'cdc-acm-console-subsystem',
        'udc_name': '40e0000.cvi-usb-dev',
        'otg_base': '0x040c0000',
        'dev_base': '0x040e0000',
        'note': 'native bare-metal endpoint driver still needs SoC-specific UDC register programming',
    },
    'files': meta,
}
(outdir / 'manifest.json').write_text(json.dumps(manifest, indent=2))
(outdir / '8051_boot_cfg.ini').write_text('[8051_BOOT]\nfirmware=mars_mcu_fw.bin\nboot_mode=ddr\nddr_addr=0x83F80000\nsize_hint_kb=512\n')
(outdir / 'load_demo.sh').write_text("#!/bin/sh\nset -eu\nmkdir -p /mnt/data\ncp bootloader.bin kernel.bin worker.bin mars_mcu_fw.bin 8051_boot_cfg.ini manifest.json /mnt/data/\necho 'Load bootloader.bin to 0x80080000'\necho 'Load kernel.bin to 0x80100000'\necho 'Load worker.bin to 0x80180000'\necho 'Load mars_mcu_fw.bin with the vendor 8051 DDR path'\necho 'Jump to bootloader.bin; it will start the 8051 and hand off to kernel.bin'\n")
os.chmod(outdir / 'load_demo.sh', 0o755)
