# nio-apps Architecture

`nio-apps` is a staging ground for portable FujiNet-NIO examples, diagnostics,
and tests. It should not become the permanent home for product configuration or
core user utilities.

The repository split direction is:

- `nio-apps`: short examples, diagnostics, smoke tests, and API exercisers.
- `nio-core-apps`: user-facing utilities such as `fdrive`, `fhost`, `fin`,
  `fout`, `fmount`, `fls`, and `fboot`.
- `nio-config`: the `config-nio` product configuration application.

The `nio-*` prefix keeps these repositories grouped together and matches the
short name used by this repository. It is intentionally less verbose than the
lower-level `fujinet-nio-lib` and `fujinet-nio` repositories.

## Application Layout

Application-owned code should live under an app-specific boundary. Shared
transport and service helpers can stay under `src/common/`, but broad shared
files should not change shape for one application through large preprocessor
branches.

`config-nio` is a core NIO configuration surface, not an example app. It now has
an app-owned layout:

- `apps/config-nio/main.c` for the application entry point.
- `apps/config-nio/include/` for app-owned public headers.
- `apps/config-nio/common/` for app-owned cross-platform code.
- `apps/config-nio/platform/*` for platform UI and storage specialisation.
- `src/common/` only for code reused by more than one application.

`config-nio` still builds through the repository-level makefiles, but its source
tree is now isolated enough to become its own repository once packaging and CI
consume explicit app artifacts rather than source globs.

## Build Layout Direction

The old TODO covered a real structural problem: too many programs were loose
files whose application identity existed only in the shared makefile. The first
cleanup step is now in place:

- `apps/core/` holds portable C implementations of core utilities until
  `nio-core-apps` exists.
- `apps/test/` holds examples, smoke tests, and diagnostics that belong in
  `nio-apps`.
- `apps/config-nio/` holds config code until `nio-config` exists.

The chosen long-term build direction is:

- Each substantial application should own its source directory, headers, common
  code, platform code, generated assets, and small build description.
- Repository-level makefiles should compose applications and platform disks;
  they should not know internal file lists for every app.
- Repo boundaries should carry product meaning: core utilities and config are
  not test apps.

`config-nio` is the first application moved into that structure because it is a
core product surface and the most likely candidate for a later repository split.
The next cleanup step is to create `nio-core-apps` and `nio-config`, then move
the staged source trees out rather than letting this repository become another
mixed product bucket.

## BBC Boot Disk

BBC boot disks are product boot disks, not portable example disks. The BBC
product uses hand-written `fn-rom` transient utilities because they are much
smaller and call the resident ROM ABI.

For that reason, BBC builds do not build the portable C implementations of:

- `fapp`
- `fdrive`
- `fhost`
- `fin`
- `fls`
- `fmount`
- `fout`

Those programs remain useful for MS-DOS and Linux targets, where memory is less
constrained and there is no BBC ROM ABI split.

The `nio-apps` BBC boot manifest stages only applications genuinely owned here:

- `CONFNIO`
- `CLOCK`
- `KEYCODE`

The full BBC product boot disk is assembled by the workspace/fn-rom flow by
combining those application assets with the `fn-rom` transient utilities.
