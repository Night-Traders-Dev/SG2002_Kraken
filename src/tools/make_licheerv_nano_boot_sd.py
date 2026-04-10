#!/usr/bin/env python3
import argparse
import pathlib
import re
import subprocess
import sys
import textwrap


CONSTANT_NAMES = (
    "BOOTLOADER_LOAD_ADDR",
    "KERNEL_LOAD_ADDR",
    "WORKER_LOAD_ADDR",
    "FW8051_DDR_ADDR",
)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Build a vendor-style boot.sd FIT image for LicheeRV Nano RISC-V boot"
    )
    parser.add_argument("--mkimage", default="mkimage")
    parser.add_argument("--package-dir", required=True)
    parser.add_argument("--kraken-header", required=True)
    parser.add_argument("--dtb", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--its-out")
    parser.add_argument(
        "--config",
        action="append",
        dest="configs",
        default=[],
        help="FIT configuration name to emit (repeatable)",
    )
    return parser.parse_args()


def die(message):
    print(f"[make_licheerv_nano_boot_sd] ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


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
            die(f"missing {name} in {header_path}")
        values[name] = int(match.group(1), 16)
    return values


def fit_path_string(path):
    return str(path.resolve()).replace("\\", "\\\\").replace('"', '\\"')


def render_its(package_dir, dtb_path, consts, configs):
    files = {
        "bootloader": package_dir / "bootloader.bin",
        "kernel": package_dir / "kernel.bin",
        "worker": package_dir / "worker.bin",
        "watch8051": package_dir / "mars_mcu_fw.bin",
    }
    for label, path in files.items():
        if not path.is_file():
            die(f"required package file missing for {label}: {path}")

    config_names = configs or ["config-sg2002_licheervnano_sd"]
    config_blocks = []
    for config_name in config_names:
        config_blocks.append(
            textwrap.indent(
                textwrap.dedent(
                    f"""\
                    {config_name} {{
                        description = "boot Kraken firmware with board {config_name}";
                        kernel = "bootloader-1";
                        fdt = "fdt-sg2002_licheervnano_sd";
                        loadables = "kraken_kernel", "kraken_worker", "kraken_watch8051";
                    }};
                    """
                ).rstrip(),
                " " * 8,
            )
        )

    return textwrap.dedent(
        f"""\
        /dts-v1/;

        / {{
            description = "Kraken LicheeRV Nano W boot FIT";
            #address-cells = <2>;

            images {{
                bootloader-1 {{
                    description = "Kraken bootloader";
                    data = /incbin/("{fit_path_string(files["bootloader"])}");
                    type = "kernel";
                    arch = "riscv";
                    os = "linux";
                    compression = "none";
                    load = <0x0 0x{consts["BOOTLOADER_LOAD_ADDR"]:08x}>;
                    entry = <0x0 0x{consts["BOOTLOADER_LOAD_ADDR"]:08x}>;
                    hash-1 {{
                        algo = "crc32";
                    }};
                }};

                kraken_kernel {{
                    description = "Kraken kernel";
                    data = /incbin/("{fit_path_string(files["kernel"])}");
                    type = "firmware";
                    arch = "riscv";
                    compression = "none";
                    load = <0x0 0x{consts["KERNEL_LOAD_ADDR"]:08x}>;
                    entry = <0x0 0x{consts["KERNEL_LOAD_ADDR"]:08x}>;
                    hash-1 {{
                        algo = "sha256";
                    }};
                }};

                kraken_worker {{
                    description = "Kraken worker";
                    data = /incbin/("{fit_path_string(files["worker"])}");
                    type = "firmware";
                    arch = "riscv";
                    compression = "none";
                    load = <0x0 0x{consts["WORKER_LOAD_ADDR"]:08x}>;
                    entry = <0x0 0x{consts["WORKER_LOAD_ADDR"]:08x}>;
                    hash-1 {{
                        algo = "sha256";
                    }};
                }};

                kraken_watch8051 {{
                    description = "Kraken 8051 watchdog firmware";
                    data = /incbin/("{fit_path_string(files["watch8051"])}");
                    type = "firmware";
                    arch = "or1k";
                    compression = "none";
                    load = <0x0 0x{consts["FW8051_DDR_ADDR"]:08x}>;
                    entry = <0x0 0x{consts["FW8051_DDR_ADDR"]:08x}>;
                    hash-1 {{
                        algo = "sha256";
                    }};
                }};

                fdt-sg2002_licheervnano_sd {{
                    description = "cvitek device tree - sg2002_licheervnano_sd";
                    data = /incbin/("{fit_path_string(dtb_path)}");
                    type = "flat_dt";
                    arch = "riscv";
                    compression = "none";
                    hash-1 {{
                        algo = "sha256";
                    }};
                }};
            }};

            configurations {{
                default = "{config_names[0]}";

        {chr(10).join(config_blocks)}
            }};
        }};
        """
    )


def main():
    args = parse_args()
    package_dir = pathlib.Path(args.package_dir)
    header_path = pathlib.Path(args.kraken_header)
    dtb_path = pathlib.Path(args.dtb)
    out_path = pathlib.Path(args.out)
    its_path = pathlib.Path(args.its_out) if args.its_out else out_path.with_suffix(
        out_path.suffix + ".its"
    )

    if not dtb_path.is_file():
        die(f"required DTB missing: {dtb_path}")

    consts = read_constants(header_path)
    its_text = render_its(package_dir, dtb_path, consts, args.configs)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    its_path.write_text(its_text, encoding="utf-8")

    subprocess.run(
        [args.mkimage, "-f", str(its_path), str(out_path)],
        check=True,
    )


if __name__ == "__main__":
    main()
