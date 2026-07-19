# MS-DOS NIO applications

These are small MS-DOS applications for `fujinet-nio`.

The `NIO*` tools link `fujinet-nio-lib` with the MS-DOS serial backend and speak
the clean NIO service protocol directly on a PC COM port. The `F*` tools link
the same library with the MS-DOS IOCTL backend and use the NIO build of
`FUJINET.SYS` as the resident DOS integration layer, so they share current
host/path state and DOS drive-to-slot mappings across command invocations.

Both paths use the same `fujinet-nio-lib` raw FujiBus/NIO call layer. The app
tree no longer carries its own SLIP, checksum, or FujiBus framing
implementation.

## Build

The shared `F*` applications now build from the repository root:

```sh
make TARGET=msdos FUJINET_NIO_LIB=../fujinet-nio-lib
```

The output executables are written to `build/msdos/bin`.

The `msdos/Makefile` remains as a compatibility wrapper:

```sh
make -C msdos FUJINET_NIO_LIB=../../fujinet-nio-lib
```

Use Open Watcom. If it is not already in the shell environment, export the
Watcom variables before building, for example:

```sh
WATCOM=/opt/watcom EDPATH=/opt/watcom/eddat INCLUDE=/opt/watcom/h PATH="/opt/watcom/binl64:/opt/watcom/binl:$PATH" make TARGET=msdos FUJINET_NIO_LIB=../fujinet-nio-lib
```

The makefile builds the required `fujinet-nio-lib` MS-DOS backend archives
automatically:

- `fujinet-nio-msdos-ioctl.lib` for `FHOST.EXE`, `FLS.EXE`, `FIN.EXE`,
  `FOUT.EXE`, `FMOUNT.EXE`, `FDRIVE.EXE`, and `FAPP.EXE`.

The old `NIOPROBE.EXE` and `NIOREAD.EXE` direct serial diagnostics are retired
from disk images. Their source may remain in `msdos/apps/` for reference, but
current MS-DOS workflows should use `FUJINET.SYS` plus the `F*` utilities.

`FLS` asks the file service for up to 420 bytes of directory payload per call by
default. That keeps normal machines from doing unnecessary extra round trips.
For a slow or fragile serial backend, build only that test disk with a smaller
page size:

```sh
make TARGET=msdos FNSVC_LIST_MAX_PAYLOAD=96 FUJINET_NIO_LIB=../fujinet-nio-lib
```

## Resident Driver Setup

Build and boot MS-DOS with the NIO driver from `fujinet-msdos`:

```sh
cd /home/markf/dev/msdos/fujinet-msdos
make -C sys FUJINET_TRANSPORT=NIO
```

In DOS, load the driver from `CONFIG.SYS`:

```dos
DEVICE=FUJINET.SYS
LASTDRIVE=Z
```

`FUJINET.SYS` creates a fixed run of DOS block drives at boot. The exact letters
depend on the DOS machine; use `FDRIVE` to see them.

The NIO driver also accepts DOS 2.x-safe `CONFIG.SYS` options for serial speed
and disk batching:

```dos
DEVICE=FUJINET.SYS FUJI_PORT=1 FUJI_BPS=115200 FUJI_BATCH_SECTORS=16 FUJI_IO_RETRIES=2
```

Keep the 16-sector disk batch default for modern emulators and real machines
where the serial path is reliable. It is what makes hosted-disk `DIR` operations
usable without speculative over-read.
For 86Box XT/8250 named-pipe testing, first try only lowering the baud rate:

```dos
DEVICE=FUJINET.SYS FUJI_PORT=1 FUJI_BPS=19200 FUJI_BATCH_SECTORS=16 FUJI_IO_RETRIES=2
```

Only reduce `FUJI_BATCH_SECTORS` for a specific backend that proves it cannot
tolerate larger framed responses.

By default the driver also enables adaptive downshift. It still starts with the
configured batch size, but if a large SLIP frame is truncated or fails
checksum it reduces future read batches to half the failed size and retries the
same DOS request. Set `FUJI_AUTO_DOWNSHIFT=0` to disable that behavior, or
`FUJI_DEBUG_IO=1` to print each recovered retry while debugging.

## Tools

- `FHOST.EXE [uri]`
  - Shows or sets the resident current FujiNet URI.
- `FLS.EXE [path]`
  - Lists the current URI or a path relative to it.
- `FIN.EXE [slot] image-uri-or-path`
  - Persists an image URI into a FujiNet mount slot. `slot` defaults to `0`.
- `FOUT.EXE slot`
  - Clears a persisted FujiNet mount slot. Cleared slots are not shown by
    `FDRIVE.EXE`.
- `FMOUNT.EXE slot [drive:] [RO|RW]`
  - Live-mounts the persisted slot through DiskService and maps the DOS drive
    unit to that slot. If `drive:` is omitted, the tool uses the FujiNet drive
    unit with the same number as the slot.
- `FDRIVE.EXE`
  - Shows FujiNet DOS drive letters, mapped NIO slots, and persisted images.
- `FAPP.EXE`
  - Exercises the `fujinet-nio-lib` app-store API through `FUJINET.SYS`.
  - Commands:
    - `FAPP LIST ns`
    - `FAPP STAT ns key`
    - `FAPP GET ns key`
    - `FAPP PUT ns key value`
    - `FAPP DEL ns key`
The `F*` tools do not touch the UART directly. They use DOS block-device IOCTL
against `FUJINET.SYS`, which keeps one owner of COM1 while allowing applications
loaded from FujiNet DOS drives to make FileService, DiskService, and network
requests.

The disk slot argument is the NIO DiskService slot number. Slot `1` is the
first configured disk mount in `fujinet-nio`.

## Example

```dos
FHOST tnfs://foo.bar/baz/
FLS
FAPP PUT config-ng colour.preference blue
FAPP GET config-ng colour.preference
FAPP LIST config-ng
FIN 0 images/DOS622.IMG
FMOUNT 0 D: RW
D:
DIR
FOUT 0
```

The command tools use 0-based FujiNet mount slots for user interaction. The
driver translates those to DiskService's current 1-based wire slot where needed.

## Creating MS-DOS Disk Images

Use `msdos/scripts/create_msdos_img.py` to create raw FAT disk images that
`fujinet-nio` can mount as DOS drives. The script uses `mkfs.fat` and `mcopy`
from `dosfstools`/`mtools`.

```sh
cd /home/markf/dev/nio/repos/nio-apps
msdos/scripts/create_msdos_img.py \
  -i build/msdos/bin \
  -o ~/8bit/TNFS/msdos/nio-apps.img \
  -l NIOAPPS
```

The image is a raw FAT volume, not qcow. That is intentional: it is the form
`fujinet-nio` DiskService can expose directly to MS-DOS.

By default the script creates a 1440 KiB FAT12 floppy-style image. This is
deliberately small because DOS directory operations read FAT and root-directory
sectors over the FujiNet block path. Use `--preset 2880` for a 2.88 MiB image,
or `--size-mb N --fat 16` when you actually need a larger hard-disk-style
volume. Size-based FAT16 images default to 8 sectors per cluster to keep FAT
metadata smaller; override with `--cluster-sectors N` if you need a different
layout.

For example, this creates a 16 MiB FAT16 image with 4 KiB clusters:

```sh
msdos/scripts/create_msdos_img.py \
  -i build/msdos/bin \
  -o ~/8bit/TNFS/msdos/nio-apps-16mb.img \
  --size-mb 16 \
  --fat 16 \
  --cluster-sectors 8 \
  -l NIO16MB
```

Avoid very small FAT16 images for MS-DOS driver testing. An 8 MiB FAT16 image
can be made valid enough for `mkfs.fat`, `mcopy`, and `mdir`, for example with
`--cluster-sectors 1` or `--cluster-sectors 2`, but MS-DOS may raise `Divide
overflow` when `DIR` asks the FujiNet block driver to build the BPB and then
walk the FAT/root directory. The FujiNet transport is not failing in that case:
the log shows ordinary 512-byte sector reads, usually followed by DOS crashing
while using the small FAT16 BPB values.

For an 8 MiB test image, use FAT12 with 4 sectors per cluster:

```sh
msdos/scripts/create_msdos_img.py \
  -i build/msdos/bin \
  -o ~/8bit/TNFS/msdos/nio-apps-8mb.img \
  --size-mb 8 \
  --fat 12 \
  --cluster-sectors 4 \
  -l NIO8MB
```

For FAT16 compatibility tests, use 16 MiB or larger. The default FAT16
`--cluster-sectors 8` layout is intended for those larger images, not for 8 MiB.

QEMU-compatible tools for this workflow are:

- `mkfs.fat` to format raw FAT12/FAT16 volumes.
- `mcopy`, `mmd`, and `mdir` to populate and inspect the image without mounting.
- `qemu-img` when you need to convert raw images to/from qcow2 for emulator boot
  disks. FujiNet drive images should stay raw unless you specifically need a
  QEMU hard-disk image.

After copying the image under your TNFS root, mount it from DOS:

```dos
FHOST tnfs://192.168.1.101/msdos/
FIN 0 nio-apps.img
FMOUNT 0 D: RW
D:
DIR
```

Input filenames must be MS-DOS 8.3 compatible. `.inf` sidecar files, hidden
dotfiles, and editor backup files ending in `~` are ignored.
