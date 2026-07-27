#!/usr/bin/env python3
"""Generate MODE 7 templates and compressed data for BBC config-nio."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

WIDTH = 40
HEIGHT = 25
OUT_DIR = Path(__file__).resolve().parents[1] / "assets"
BBC_DIR = Path(__file__).resolve().parents[1]
SRC_BBC_DIR = Path(__file__).resolve().parents[2] / "src" / "platform" / "bbc"
DEFAULT_LAYOUT = BBC_DIR / "config_nio_layout.json"


def load_layout(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as f:
        layout = json.load(f)
    screen = layout["screen"]
    if screen["width"] != WIDTH or screen["height"] != HEIGHT:
        raise ValueError(f"{path}: only {WIDTH}x{HEIGHT} MODE 7 layouts are supported")
    return layout


def blank() -> list[list[int]]:
    return [[ord(" ") for _ in range(WIDTH)] for _ in range(HEIGHT)]


def put(screen: list[list[int]], x: int, y: int, text: str) -> None:
    for i, ch in enumerate(text[: WIDTH - x]):
        screen[y][x + i] = ord(ch)


def hline(screen: list[list[int]], y: int, x: int, width: int) -> None:
    put(screen, x, y, "+" + ("-" * (width - 2)) + "+")


def box(screen: list[list[int]], x: int, y: int, width: int, height: int) -> None:
    hline(screen, y, x, width)
    for row in range(y + 1, y + height - 1):
        screen[row][x] = ord("|")
        screen[row][x + width - 1] = ord("|")
    hline(screen, y + height - 1, x, width)


def common(screen: list[list[int]], title: str, layout: dict) -> None:
    put(screen, 0, 0, " " * WIDTH)
    put(screen, 1, 0, "CONFNIO")
    put(screen, 28, 0, title)
    put(screen, 0, 1, "H Hosts   S Slots   X Mount   Q Quit")
    hline(screen, 2, 0, WIDTH)
    hline(screen, 21, 0, WIDTH)
    put(screen, layout["screen"]["status"]["x"], layout["screen"]["status"]["y"], " " * layout["screen"]["status"]["width"])
    footer = layout["screen"]["footer"]
    put(screen, footer["x"], footer["y"], footer["text"])
    put(screen, 0, 24, " " * WIDTH)


def hosts(layout: dict) -> list[list[int]]:
    screen = blank()
    common(screen, "HOSTS", layout)
    put(screen, 1, 3, "Hosts page 0-7")
    box(screen, 0, 5, WIDTH, 10)
    rows = layout["hosts"]["rows"]
    for row in range(rows["count"]):
        put(screen, rows["x"] + 1, rows["y"] + row, " __ _______________________________")
    put(screen, 1, 16, "Left/Right change host page")
    put(screen, 1, 18, "RET browse   A add   E edit   D del")
    input_field = layout["hosts"]["input"]
    put(screen, 0, input_field["y"], "  Host:" + (" " * 33))
    return screen


def browse(layout: dict) -> list[list[int]]:
    screen = blank()
    common(screen, "BROWSE", layout)
    put(screen, 1, 4, "Host:")
    put(screen, 1, 5, "Path:")
    put(screen, 1, 19, "RET open dir/file   A assign   U up")
    input_field = layout["browse"]["input"]
    put(screen, 0, input_field["y"], "  Slot:" + (" " * 33))
    return screen


def slots(layout: dict) -> list[list[int]]:
    screen = blank()
    common(screen, "SLOTS", layout)
    put(screen, 1, 3, "Drive mappings")
    put(screen, 1, 11, "Slots")
    return screen


def screen_to_bytes(screen: list[list[int]]) -> bytes:
    return bytes(cell for row in screen for cell in row)


def bytes_to_screen(name: str, data: bytes) -> list[list[int]]:
    if len(data) != WIDTH * HEIGHT:
        raise ValueError(f"{name}: expected {WIDTH * HEIGHT} bytes, got {len(data)}")
    return [list(data[row * WIDTH : (row + 1) * WIDTH]) for row in range(HEIGHT)]


def write_asset(name: str, data: bytes) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    if len(data) != WIDTH * HEIGHT:
        raise ValueError(f"{name}: expected {WIDTH * HEIGHT} bytes, got {len(data)}")
    (OUT_DIR / name).write_bytes(data)


def find_best_seq(data: bytes, dst: int, max_offset: int = 256, max_seq_len: int = 129) -> tuple[int, int]:
    best_from = 0
    best = 0
    for src in range(max(dst - max_offset, 0), dst):
        limit = min(len(data) - dst, max_seq_len)
        for num in range(limit):
            if data[src + num] != data[dst + num]:
                break
            matched = num + 1
            if matched > best:
                best_from = src
                best = matched
    return best_from, best


def compress(data: bytes) -> bytes:
    out = bytearray()
    dst = 0
    raw_copy_len = 0
    raw_len_addr = 0
    while dst < len(data):
        src, best = find_best_seq(data, dst)
        if best >= 2 + (1 if raw_copy_len else 0):
            if raw_copy_len:
                out[raw_len_addr] = raw_copy_len
                raw_copy_len = 0
            out.append((best - 2) | 0x80)
            out.append((src - dst + 0x100) & 0xFF)
            dst += best
        else:
            if raw_copy_len == 127:
                out[raw_len_addr] = raw_copy_len
                raw_copy_len = 0
            if not raw_copy_len:
                raw_len_addr = len(out)
                out.append(0)
            out.append(data[dst])
            raw_copy_len += 1
            dst += 1
    if raw_copy_len:
        out[raw_len_addr] = raw_copy_len
    out.append(0)
    return bytes(out)


def macro_name(*parts: str) -> str:
    return "CONFIG_NIO_BBC_" + "_".join(parts).upper()


def write_layout_header(layout: dict) -> None:
    out = SRC_BBC_DIR / "config_nio_layout.h"
    lines = [
        "/* Generated by bbc/scripts/generate_config_nio_templates.py */",
        "#ifndef CONFIG_NIO_BBC_LAYOUT_H",
        "#define CONFIG_NIO_BBC_LAYOUT_H",
        "",
    ]

    def emit(prefix: tuple[str, ...], value) -> None:
        if isinstance(value, dict):
            for key, child in value.items():
                emit((*prefix, key), child)
        elif isinstance(value, int):
            lines.append(f"#define {macro_name(*prefix)} {value}")

    emit((), layout)
    lines.extend([
        "",
        "#endif",
        "",
    ])
    out.write_text("\n".join(lines), encoding="utf-8")


def write_template_data(assets: dict[str, bytes]) -> None:
    out = SRC_BBC_DIR / "config_nio_template_data.s"
    labels = {
        "CNHOSTS": "_config_nio_tpl_hosts",
        "CNBROW": "_config_nio_tpl_browse",
        "CNSLOTS": "_config_nio_tpl_slots",
    }
    lines = [
        "; Generated by bbc/scripts/generate_config_nio_templates.py",
        "        .export _config_nio_tpl_hosts",
        "        .export _config_nio_tpl_browse",
        "        .export _config_nio_tpl_slots",
        "",
        "        .rodata",
    ]
    for name, data in assets.items():
        packed = compress(data)
        lines.append("")
        lines.append(f"{labels[name]}:")
        for offset in range(0, len(packed), 16):
            chunk = ", ".join(f"${byte:02X}" for byte in packed[offset : offset + 16])
            lines.append(f"        .byte {chunk}")
    out.write_text("\n".join(lines) + "\n")


def read_input_or_placeholder(path: Path | None, name: str, generated: bytes) -> bytes:
    if path is None:
        return generated
    data = path.read_bytes()
    if len(data) != WIDTH * HEIGHT:
        raise ValueError(f"{path}: {name} input must be exactly {WIDTH * HEIGHT} bytes, got {len(data)}")
    return data


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--layout", type=Path, default=DEFAULT_LAYOUT,
                        help="JSON layout source for dynamic overlay coordinates")
    parser.add_argument("-t", "--hosts", type=Path,
                        help="1000-byte MODE 7 hosts template input")
    parser.add_argument("-b", "--browse", type=Path,
                        help="1000-byte MODE 7 browse template input")
    parser.add_argument("-s", "--slots", type=Path,
                        help="1000-byte MODE 7 slots template input")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    layout = load_layout(args.layout)
    assets = {
        "CNHOSTS": read_input_or_placeholder(args.hosts, "CNHOSTS", screen_to_bytes(hosts(layout))),
        "CNBROW": read_input_or_placeholder(args.browse, "CNBROW", screen_to_bytes(browse(layout))),
        "CNSLOTS": read_input_or_placeholder(args.slots, "CNSLOTS", screen_to_bytes(slots(layout))),
    }
    for name, data in assets.items():
        write_asset(name, data)
    write_template_data(assets)
    write_layout_header(layout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
