# nio-apps

A repository for FujiNet-NIO example, diagnostic, and test applications.

Product utilities are being split out of this repository. This repo's purpose is
to hold short examples and applications that exercise FujiNet-NIO functionality
without becoming product surfaces themselves.

## Layout

- `apps/core/` temporarily contains portable C implementations of core `F*`
  utilities while they wait for migration to a core-app repository.
- `apps/test/` contains examples, smoke tests, and diagnostic applications.
- `apps/config-nio/` contains the product configuration application and its
  app-owned shared/platform code while it waits for migration to a config
  repository.
- `src/common/` contains shared app support code.
- `src/platform/<platform>/` contains platform backends and any platform-tuned
  code.
- `makefiles/` contains the target/compiler/disk build fragments.
- `msdos/` contains MS-DOS-only tools, scripts, and compatibility entry points.

The portable C core `F*` applications currently build for:

- `linux`, using GCC and the Linux NIO backend.
- `msdos`, using Open Watcom and the MS-DOS IOCTL backend.
- `atari`, using cc65 and the Atari FujiBus backend.

BBC product builds use the smaller `fn-rom` ASM transient utilities for `F*`
commands instead. See `docs/ARCHITECTURE.md`.

## Build

Build all configured targets:

```sh
make
```

Build one target:

```sh
make TARGET=msdos FUJINET_NIO_LIB=../fujinet-nio-lib
make TARGET=atari FUJINET_NIO_LIB=../fujinet-nio-lib
```

The explicit target for all configured platforms is also available:

```sh
make all-targets FUJINET_NIO_LIB=../fujinet-nio-lib
```

Outputs are written to `build/<target>/bin/`:

- MS-DOS: `.exe`
- Atari: `.xex`
- BBC: extensionless program binaries

Portable apps are discovered from `apps/core/*.c` and `apps/test/*.c`.
MS-DOS-specific apps are discovered from `msdos/apps/*.c`. `config-nio` is
intentionally outside both portable app buckets under `apps/config-nio/`, so its
source tree can move to its own repository without untangling generic app code
first.

Future app moves should prefer an app-owned directory with its own small build
description over adding more top-level source files that are resolved only by
repository-wide makefile rules.

The build invokes `fujinet-nio-lib` for the target-specific raw NIO library
when needed.

## HTTPBin Smoke App

`fhttpbin` exercises FujiNet-NIO network sessions against an httpbin-compatible
service. Start the local service from `fujinet-nio` with:

```sh
../fujinet-nio/scripts/start_test_services.sh http
```

MS-DOS can pass the base URL directly or use `FN_HTTPBIN_URL`:

```sh
FHTTPBIN http://127.0.0.1:8080
```

cc65 targets use the compile-time `FN_DEFAULT_HTTPBIN_URL` value so emulator
smoke tests can run unattended. Build with `-DFHTTPBIN_PROMPT_URL` if an
interactive URL prompt is wanted for a manual target build.

### Running atari version via CLI into Altirra/Embedded

```shell
repos/fujinet-nio/scripts/start_test_services.sh http
ATARI_OS_ROMS="$HOME/8bit/atari/images/os" \
ATARI_BASIC_ROMS="$HOME/8bit/atari/images/atari-basic" \
  scripts/build.sh atari-run altirra --profile configs/atari/profiles/altirra-embedded-fhttpbin.yaml
```

## App-Store Smoke App

`astest` performs fixed app-store CRUD checks against namespace `nio.astest`.
It writes and verifies `alpha`, writes and verifies chunked `beta`, lists both
keys, then deletes `beta` and leaves `alpha` behind for filesystem inspection.

In embedded AltirraSDL runs, the FujiNet-NIO `host:` filesystem is the generated
`fujinet-data` directory printed by `scripts/build.sh`. The remaining data file
is expected at:

```text
<run-root>/fujinet-data/FujiNet/app-store/v1/nio.astest/alpha.bin
```

Run the Atari version with:

```shell
ATARI_OS_ROMS="$HOME/8bit/atari/images/os" \
ATARI_BASIC_ROMS="$HOME/8bit/atari/images/atari-basic" \
  scripts/build.sh atari-run altirra --profile configs/atari/profiles/altirra-embedded-astest.yaml
```

The Atari build waits for a key before exit so the temporary data directory can
be inspected before the runner cleans it up.

## Disk Images

Disk-image packaging is target-specific:

```sh
make TARGET=msdos disk FUJINET_NIO_LIB=../fujinet-nio-lib
make TARGET=atari disk FUJINET_NIO_LIB=../fujinet-nio-lib
make TARGET=bbc disk FUJINET_NIO_LIB=../fujinet-nio-lib
```

The MS-DOS disk target creates `build/msdos/disk/nio-apps-msdos.img`.

The Atari disk target creates `build/atari/disk/nio-apps-atari.atr` with
`dir2atr`. It stages `.xex` files under `build/atari/disk/stage` and caches
`picoboot.bin` under `build/atari/cache/atari`. Install `dir2atr` from
AtariSIO or set `DIR2ATR=/path/to/dir2atr` when invoking make.

## Boot Disks

The `boot-disk/` project creates small platform config disks from files listed
in `boot-disk/manifests/<platform>.yaml`.

```sh
make TARGET=msdos boot-disk
make TARGET=atari boot-disk
make TARGET=bbc boot-disk
make boot-disk-all
```

The platform manifest controls the disk contents:

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

Generated images are written under `build/<target>/disk/boot/`:

- MS-DOS: `autorun.img`
- Atari: `autorun.atr`
- BBC: `FN-BOOT.ssd`

To seed a neighboring `fujinet-nio` checkout:

```sh
make TARGET=atari install-boot-disk FUJINET_NIO=../fujinet-nio
```

That installs to `distfiles/esp32-data/boot/<platform>/`, which is the
filesystem input packaged by `fujinet-nio` when running `./build.sh -f`.

For BBC, this repository's boot image contains only `CONFNIO`, `CLOCK`, and
`KEYCODE`. The product boot disk combines those app assets with the smaller
`fn-rom` ASM transient utilities.
