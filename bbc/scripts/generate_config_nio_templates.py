#!/usr/bin/env python3
"""Generate placeholder MODE 7 templates for BBC config-nio."""

from __future__ import annotations

from pathlib import Path

WIDTH = 40
HEIGHT = 25
OUT_DIR = Path(__file__).resolve().parents[1] / "assets"


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


def common(screen: list[list[int]], title: str) -> None:
    put(screen, 0, 0, " " * WIDTH)
    put(screen, 1, 0, "CONFNIO")
    put(screen, 28, 0, title)
    put(screen, 0, 1, "H Hosts   S Slots   X Mount   Q Quit")
    hline(screen, 2, 0, WIDTH)
    hline(screen, 21, 0, WIDTH)
    put(screen, 1, 22, " " * 38)
    put(screen, 0, 23, "BBC Master MODE 7")
    put(screen, 0, 24, " " * WIDTH)


def hosts() -> list[list[int]]:
    screen = blank()
    common(screen, "HOSTS")
    put(screen, 1, 3, "Hosts page 0-7")
    box(screen, 0, 5, WIDTH, 10)
    for row in range(8):
        put(screen, 2, 6 + row, "  __ _______________________________")
    put(screen, 1, 16, "Left/Right change host page")
    put(screen, 1, 18, "RET browse   A add   E edit   D del")
    put(screen, 0, 20, chr(147) + " Host  " + chr(149) + (" " * 30) + chr(149))
    return screen


def browse() -> list[list[int]]:
    screen = blank()
    common(screen, "BROWSE")
    put(screen, 1, 4, "Host:")
    put(screen, 1, 5, "Path:")
    box(screen, 0, 6, WIDTH, 12)
    for row in range(10):
        put(screen, 2, 7 + row, "   __________________________________")
    put(screen, 1, 19, "RET open dir/file   A assign   U up")
    put(screen, 0, 20, chr(147) + " Slot  " + chr(149) + (" " * 30) + chr(149))
    return screen


def slots() -> list[list[int]]:
    screen = blank()
    common(screen, "SLOTS")
    put(screen, 1, 3, "Drive mappings")
    box(screen, 0, 4, WIDTH, 6)
    for row in range(4):
        put(screen, 2, 5 + row, " Drive_ S_ _ ________________________")
    put(screen, 1, 11, "Slots")
    box(screen, 0, 11, WIDTH, 10)
    for row in range(8):
        put(screen, 2, 12 + row, "  _ _________________________________")
    return screen


def write_asset(name: str, screen: list[list[int]]) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    data = bytes(cell for row in screen for cell in row)
    if len(data) != WIDTH * HEIGHT:
        raise ValueError(f"{name}: expected {WIDTH * HEIGHT} bytes, got {len(data)}")
    (OUT_DIR / name).write_bytes(data)


def main() -> int:
    write_asset("CNHOSTS", hosts())
    write_asset("CNBROW", browse())
    write_asset("CNSLOTS", slots())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
