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

Build one target:

```sh
make TARGET=msdos FUJINET_NIO_LIB=../fujinet-nio-lib
make TARGET=atari FUJINET_NIO_LIB=../fujinet-nio-lib
```

Build all configured targets:

```sh
make all-targets FUJINET_NIO_LIB=../fujinet-nio-lib
```

Outputs are written to `bin/<target>/`:

- MS-DOS: `fhost.exe`, `fls.exe`, `fin.exe`, `fmount.exe`, `fdrive.exe`,
  `fapp.exe`
- Atari: `fhost.xex`, `fls.xex`, `fin.xex`, `fmount.xex`, `fdrive.xex`,
  `fapp.xex`

The build invokes `fujinet-nio-lib` for the target-specific raw NIO library when
needed.

## Disk Images

Disk-image packaging is target-specific:

```sh
make -f makefiles/build.mk TARGET=msdos disk FUJINET_NIO_LIB=../fujinet-nio-lib
make -f makefiles/build.mk TARGET=atari disk FUJINET_NIO_LIB=../fujinet-nio-lib
```

The MS-DOS disk target creates `disk-images/nio-apps-msdos.img`. The Atari disk
target currently stages `.xex` files under `disk-images/atari-stage`; ATR
creation is intentionally isolated to `makefiles/disk-atari.mk` so the final
tooling can be selected without changing the common build rules.
