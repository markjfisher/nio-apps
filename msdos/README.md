# MS-DOS NIO applications

These are small MS-DOS test applications for `fujinet-nio`. They speak the
clean NIO service protocol over FujiBus + SLIP on a PC COM port. They are kept
outside the legacy `fujinet-msdos` tree so they can exercise `fujinet-nio`
directly without depending on the legacy Atari-style FujiNet protocol surface.

## Build

Use Open Watcom. If it is not already in the shell environment:

```sh
WATCOM=/opt/watcom EDPATH=/opt/watcom/eddat INCLUDE=/opt/watcom/h PATH="/opt/watcom/binl64:/opt/watcom/binl:$PATH" make -C msdos
```

The output executables are written to `msdos/bin`.

## Tools

- `NIOPROBE.EXE [slot] [com]`
  - Calls DiskService `Info` and prints the mounted image status.
  - Defaults to NIO disk slot `1` and `COM1`.
- `NIOREAD.EXE [slot] [lba] [bytes] [com]`
  - Reads a sector and prints a hex dump.
  - Defaults to slot `1`, LBA `0`, `512` bytes, and `COM1`.

The disk slot argument is the NIO DiskService slot number. Slot `1` is the
first configured disk mount in `fujinet-nio`.
