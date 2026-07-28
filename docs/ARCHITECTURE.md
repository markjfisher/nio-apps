# nio-apps Architecture

`nio-apps` is a staging ground for portable FujiNet-NIO examples, diagnostics,
and tests. It is not the home for product configuration or core user utilities.

The repository split is:

- `nio-apps`: short examples, diagnostics, smoke tests, and API exercisers.
- `nio-core-apps`: user-facing utilities such as `fapp`, `fboot`, `fdrive`,
  `fhost`, `fin`, `fout`, `fmount`, and `fls`.
- `nio-config`: the `config-nio` product configuration application.

The `nio-*` prefix keeps these repositories grouped together and matches the
short name used by this repository.

## Boundaries

Code in this repository should be useful for exercising or demonstrating an API,
but should not be a required user utility or boot-disk component.

Core utilities and their platform boot disks belong to `nio-core-apps`.

Configuration UI, config assets, and config integration tests belong to
`nio-config`.

BBC product boot disks are assembled outside this repo from `fn-rom` core
utilities and `nio-config` config assets.
