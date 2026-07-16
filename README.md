# nio-apps

A repository for fujinet-nio test applications.

Anything serious will move out of this repo. This repo's purpose is a platform for initial applications without a plethora of separate repositories.

## Layout

- `apps/common/` contains shared command applications.
- `src/common/` contains shared app support code.
- `src/platform/<platform>/` contains platform backends and any platform-tuned
  code.
- `makefiles/` contains the target/compiler/disk build fragments.
- `msdos/` contains MS-DOS-only tools, scripts, and compatibility entry points.

The shared `F*` applications currently build for:

- `msdos`, using Open Watcom and the MS-DOS IOCTL backend.
- `atari`, using cc65 and the Atari FujiBus backend.
- `bbc` and `bbc-clib`, using cc65 and the BBC Tube/serial backends.

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

Common apps are discovered from `apps/common/*.c`. MS-DOS-specific apps are
discovered from `msdos/apps/*.c`. Add target-specific exclusions in
`makefiles/build.mk` only when a source file cannot build for that target.

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
- BBC: `autorun.ssd`

To seed a neighboring `fujinet-nio` checkout:

```sh
make TARGET=atari install-boot-disk FUJINET_NIO=../fujinet-nio
```

That installs to `distfiles/boot/<platform>/` and
`distfiles/esp32-data/boot/<platform>/` so `fujinet-nio` boot profiles can
select the matching image with `boot.config_uri`.
