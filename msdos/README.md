# MS-DOS NIO applications

These are small MS-DOS applications for `fujinet-nio`.

The `NIO*` tools speak the clean NIO service protocol directly over FujiBus +
SLIP on a PC COM port. The `F*` tools use the NIO build of `FUJINET.SYS` as the
resident DOS integration layer, so they share current host/path state and DOS
drive-to-slot mappings across command invocations.

## Build

Use Open Watcom. If it is not already in the shell environment:

```sh
WATCOM=/opt/watcom EDPATH=/opt/watcom/eddat INCLUDE=/opt/watcom/h PATH="/opt/watcom/binl64:/opt/watcom/binl:$PATH" make -C msdos
```

The output executables are written to `msdos/bin`.

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

## Tools

- `FHOST.EXE [uri]`
  - Shows or sets the resident current FujiNet URI.
- `FLS.EXE [path]`
  - Lists the current URI or a path relative to it.
- `FIN.EXE [slot] image-uri-or-path`
  - Persists an image URI into a FujiNet mount slot. `slot` defaults to `0`.
- `FMOUNT.EXE slot [drive:] [RO|RW]`
  - Live-mounts the persisted slot through DiskService and maps the DOS drive
    unit to that slot. If `drive:` is omitted, the tool uses the FujiNet drive
    unit with the same number as the slot.
- `FDRIVE.EXE`
  - Shows FujiNet DOS drive letters, mapped NIO slots, and persisted images.
- `NIOPROBE.EXE [slot] [com]`
  - Calls DiskService `Info` and prints the mounted image status.
  - Defaults to NIO disk slot `1` and `COM1`.
- `NIOREAD.EXE [slot] [lba] [bytes] [com]`
  - Reads a sector and prints a hex dump.
  - Defaults to slot `1`, LBA `0`, `512` bytes, and `COM1`.

The disk slot argument is the NIO DiskService slot number. Slot `1` is the
first configured disk mount in `fujinet-nio`.

## Example

```dos
FHOST tnfs://foo.bar/baz/
FLS
FIN 0 images/DOS622.IMG
FMOUNT 0 D: RW
D:
DIR
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
  -i msdos/bin \
  -o ~/8bit/TNFS/msdos/nio-apps.img \
  -l NIOAPPS
```

The image is a raw FAT volume, not qcow. That is intentional: it is the form
`fujinet-nio` DiskService can expose directly to MS-DOS.

By default the script creates a 1440 KiB FAT12 floppy-style image. This is
deliberately small because DOS directory operations read FAT and root-directory
sectors over the FujiNet block path; a mostly empty 32 MiB FAT16 image can make
simple commands like `DIR` feel slow. Use `--preset 2880` for a 2.88 MiB image,
or `--size-mb N --fat 16` when you actually need a larger hard-disk-style
volume.

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
