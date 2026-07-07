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

- MS-DOS: `fhost.exe`, `fls.exe`, `fin.exe`, `fmount.exe`, `fdrive.exe`,
  `fapp.exe`, `fhttpbin.exe`, `astest.exe`
- Atari: `fhost.xex`, `fls.xex`, `fin.xex`, `fmount.xex`, `fdrive.xex`,
  `fapp.xex`, `fhttpbin.xex`, `astest.xex`

The build invokes `fujinet-nio-lib` for the target-specific raw NIO library when
needed.

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
make -f makefiles/build.mk TARGET=msdos disk FUJINET_NIO_LIB=../fujinet-nio-lib
make -f makefiles/build.mk TARGET=atari disk FUJINET_NIO_LIB=../fujinet-nio-lib
```

The MS-DOS disk target creates `build/msdos/disk/nio-apps-msdos.img`.

The Atari disk target creates `build/atari/disk/nio-apps-atari.atr` with
`dir2atr`. It stages `.xex` files under `build/atari/disk/stage` and caches
`picoboot.bin` under `build/atari/cache/atari`. Install `dir2atr` from
AtariSIO or set `DIR2ATR=/path/to/dir2atr` when invoking make.
