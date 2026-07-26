# nio-apps TODO

## Application Structure

- Revisit the `config-nio` source layout before taking the BBC implementation much further.
- Current code is split across broad shared files such as `fnctl.c`, `fnsvc_config_nio_bbc.c`, and common `config_nio_*` modules, with `CONFIG_NIO_BBC_LITE` carving those files differently for this one application.
- Consider moving application-specific service/control wrappers into an app-owned folder, so platform and app specialisation is visible in the directory structure rather than being hidden behind preprocessor branches in shared modules.
- Keep genuinely reusable transport/service code shared, but avoid making general platform files change shape depending on which application is being linked.
