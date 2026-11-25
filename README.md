# Apache Celix as a Bazel module

This repository packages **Apache Celix** (the OSGi-inspired modular framework for C and C++, https://celix.apache.org) as a dependency-manageable Bazel module.
It is a standalone project, separate from rules_celix, the Bazel packaging ruleset that turns C/C++ shared libraries into Celix bundles.

## What this module provides

Reference the module as `@celix` (canonical name).
It exposes:

| Target                  | Kind            | Purpose                                                            |
|-------------------------|-----------------|--------------------------------------------------------------------|
| `@celix//:framework`    | headers-only    | The framework + `libs/utils` public include tree, no compiled       |
|                         | `cc_library`    | objects. Link this when bundle activators must leave `celix_*`      |
|                         |                 | symbols unresolved so they bind against a single embedded framework |
|                         |                 | instance at dlopen time (runtime link mode).                       |
| `@celix//:framework_static` | full static | The complete framework + utils archive (`alwayslink`), for          |
|                         | `cc_library`    | self-contained bundles (`framework = "static"`) and container       |
|                         |                 | runners that embed and export the framework.                       |
| `@celix//:utils`        | `cc_library`    | The compiled `libs/utils` archive (what `framework_static` builds   |
|                         |                 | on).                                                               |

All three are aliases into `@celix//framework:...` / `@celix//uuid:uuid`.

### The static / headers-only split

A process may host only **one** Celix framework instance.
Bundle activators therefore compile against the headers-only `:framework` target: their unresolved `celix_*` symbols resolve against the single embedded framework exported by a container runner at dlopen time.
Embedding a second compiled copy into the activator would break this single-instance guarantee and crash C++ activators with ODR/ABI clashes (`celix::impl::createActivator`).
The `:framework_static` archive exists for artifacts that are not opened inside a container runner: standalone runners and fully self-contained bundles.

## How consumers use it

From a bzlmod workspace:

```starlark
bazel_dep(name = "celix", version = "2.4.0")
```

Until this module is in the Bazel Central Registry, pin it with a
`local_path_override` or `git_override`:

```starlark
local_path_override(
    module_name = "celix",
    path = "/path/to/this/repo",
)
```

The framework hard-dependencies `zlib` and `libzip` are **vendored** under `third_party/` with native Bazel overlays (no CMake, no system installs), so consumers need nothing beyond a working C/C++ toolchain. `uuid/` is a small checked-in RFC 4122 unit replacing a system libuuid.

> Why not the BCR `libzip` module? The BCR libzip builds via `rules_foreign_cc` (CMake), which requires a host `cmake` toolchain and drags in boringssl/zstd.
> The vendored native overlay matches the hermetic, tested semantic baseline that rules_celix uses.

For bundling and container rules, see [rules_celix](https://github.com/softyo/rules_celix), which consumes this module as its canonical framework.

## Repository layout

```
celix/                      # repo root
├── MODULE.bazel            # module(name = "celix", version = "2.4.0")
├── BUILD.bazel             # root aliases (@celix//:framework, ...)
├── framework/
│   ├── BUILD.bazel         # the framework + utils cc_library targets
│   ├── libs/               # vendored Apache Celix 2.4.0 source snapshot
│   └── gen/                # CMake-generated headers (checked-in reference)
├── third_party/            # vendored zlib + libzip with native overlays
├── uuid/                   # hermetic libuuid replacement (RFC 4122 subset)
└── tests/                  # minimal smoke test (links framework_static)
```

## Building

```
bazel build //...
# or, individually
bazel build @celix//:framework
bazel build @celix//:framework_static
bazel build @celix//:utils
```

No CMake, no system Celix, no system libuuid is required: zlib and libzip are vendored with native overlays and `uuid/` is a small checked-in unit.

## Releases and the BCR

Releases and the Bazel Central Registry submission (source archive contract,
manual PR steps) are documented in [`docs/BCR_RELEASE.md`](docs/BCR_RELEASE.md).

## License

Apache License 2.0. The vendored source is Apache Celix
(https://github.com/apache/celix), also Apache-2.0; see `NOTICE`.
