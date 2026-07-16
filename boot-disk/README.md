# nio-apps boot-disk

This directory contains optional source files for FujiNet-NIO boot/config disk
images.

The make targets combine selected built `nio-apps` binaries and payload files
from `manifests/<platform>.yaml`, then produce platform disk images:

```sh
make TARGET=atari boot-disk
make TARGET=bbc boot-disk
make TARGET=msdos boot-disk
```

Outputs are written under `build/<target>/disk/boot/`:

- Atari: `autorun.atr`
- BBC: `autorun.ssd`
- MS-DOS: `autorun.img`

To copy the generated image into the matching FujiNet-NIO boot asset folders:

```sh
make TARGET=atari install-boot-disk FUJINET_NIO=../fujinet-nio
```

This installs to:

- `distfiles/boot/<platform>/`
- `distfiles/esp32-data/boot/<platform>/`

FujiNet-NIO platform profiles can then point `boot.config_uri` at the matching
platform image.

Files under `files/` are not copied just because they exist. List every disk
entry in the platform YAML manifest:

```yaml
apps:
  - src: ${BOOT_DISK_BIN}/fhost.exe
    name: FHOST.EXE
  - src: boot-disk/files/msdos/AUTORUN.BAT
    name: AUTOEXEC.BAT
```

Manifest `src` paths support shell-style environment expansion, including
`${BOOT_DISK_BIN}`, and relative paths are resolved from the `nio-apps`
repository root. Optional entries can use `required: false`.
