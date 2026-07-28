# nio-apps Architecture

`nio-apps` is a staging ground for portable FujiNet-NIO applications and
diagnostic tools. It should not become the permanent home for every product
surface or every platform utility.

## Application Layout

Application-owned code should live under an app-specific boundary. Shared
transport and service helpers can stay under `src/common/`, but broad shared
files should not change shape for one application through large preprocessor
branches.

`config-nio` is the main pressure point. It is a core NIO configuration surface,
not just an example app. It now has an app-owned layout:

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
files whose application identity existed only in the shared makefile. The chosen
direction is:

- Application source belongs under `apps/<name>/`.
- Each substantial application should own its headers, common code, platform
  code, generated assets, and small build description.
- Repository-level makefiles should compose applications and platform disks;
  they should not know internal file lists for every app.
- `apps/common/*.c` remains a temporary compatibility bucket for small portable
  utilities while they are migrated.

`config-nio` is the first application moved into that structure because it is a
core product surface and the most likely candidate for a later repository split.
The next cleanup step is to move each remaining utility from `apps/common/` into
`apps/<utility>/main.c` with explicit platform support metadata, then replace
the shared source glob with included per-app build descriptions.

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
