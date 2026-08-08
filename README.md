# nio-apps

A repository for FujiNet-NIO example, diagnostic, and test applications.

Product utilities have moved out of this repository. This repo's purpose is to
hold short examples and applications that exercise FujiNet-NIO functionality
without becoming product surfaces themselves.

Core `F*` utilities live in `../nio-core-apps`.

The `config-nio` product configuration application lives in `../nio-config`.

## Layout

- `apps/test/` contains examples, smoke tests, and diagnostic applications.
- `src/common/` contains shared app support code.
- `src/platform/<platform>/` contains platform backends and any platform-tuned
  code.
- `makefiles/` contains the target/compiler/disk build fragments.
- `msdos/` contains MS-DOS-only diagnostic tools, scripts, and compatibility
  entry points.

## Build

Build all configured targets:

```sh
make
```

Build one target:

```sh
make TARGET=msdos FUJINET_NIO_LIB=../fujinet-nio-lib
make TARGET=atari FUJINET_NIO_LIB=../fujinet-nio-lib
make TARGET=linux FUJINET_NIO_LIB=../fujinet-nio-lib
make TARGET=amiga FUJINET_NIO_LIB=../fujinet-nio-lib
```

Outputs are written to `build/<target>/bin/`:

- MS-DOS: `.exe`
- Atari: `.xex`
- BBC: extensionless program binaries
- Amiga: extensionless AmigaOS executables

Portable test apps are discovered from `apps/test/*.c`. MS-DOS-specific
diagnostic apps are discovered from `msdos/apps/*.c`.

`wifitest` exercises Wi-Fi status, configuration, and scan calls using a
small caller-owned scan buffer. It is intentionally read-only, so it can be
used as a smoke test without changing the adapter configuration.

The Amiga target uses `serial.device` through `fujinet-nio-lib`. Its `fnctl`
compatibility state is persisted through FujiNet's app-store service because
Amiga has no FujiNet DOS driver state interface; FujiBus calls themselves use
the normal library transport.

Boot disks are no longer owned here. Core utility boot disks belong to
`../nio-core-apps`; config disks/stages belong to `../nio-config`.

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

## App-Store Smoke App

`astest` performs fixed app-store CRUD checks against namespace `nio.astest`.
It writes and verifies `alpha`, writes and verifies chunked `beta`, lists both
keys, then deletes `beta` and leaves `alpha` behind for filesystem inspection.
