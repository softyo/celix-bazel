# Implementation notes (port from rules_celix into this module)

These notes record the decisions taken while moving the framework build out of
rules_celix into this standalone module. They are for the maintainers and for
the eventual Apache Celix upstream adoption.

## Source snapshot (Option 1: vendored tree)

The module **vendors** the Apache Celix 2.4.0 source snapshot (the framework
core + `libs/utils`), matching the plan's recommended option for upstream
adoption (an in-tree `BUILD.bazel` next to the existing CMake build, no fetch
machinery). The vendored copy lives under `framework/libs/` and is a trimmed
snapshot: only what the Bazel targets need (no CMake build files, no gtest, no
optional subsystems like DFI/remote services).

## What moved from rules_celix

- `third_party/celix/framework.BUILD` -> `framework/BUILD.bazel` (renamed
  target package, `libs/...` globs adjusted to the vendored layout).
- `third_party/celix/gen/{framework,utils}/*.h` -> `framework/gen/...`
  (reference copies) with the operative copies vendored directly into the
  `include/` trees (`celix_framework_export.h`, `celix_utils_export.h`,
  `celix_err_constants.h`), replacing the `generated_headers.patch` injection.
- `third_party/celix/uuid/` -> `uuid/` (unchanged: `cc_library` of the RFC 4122
  stub with `copts = ["-std=c11"]`, `includes = ["."]`).

## libzip / zlib: vendored overlays, not BCR deps

The plan preferred BCR `zlib@1.3.1.bcr.1` + `libzip@1.10.1`. Build time proved
the BCR `libzip` module unusable for this framework:

- The BCR libzip builds via `rules_foreign_cc` (CMake), which bootstraps a
  whole foreign toolchain, needs no host cmake present on this machine, and
  compiles a much larger tree (boringssl crypto, zstd, ...).
- Its CMake install step places the archive in `lib64/` on this host, but the
  rule declares `out_static_libs = ["libzip.a"]` under `lib/`, so the expected
  output is never produced and the build fails (`Foreign Cc - CMake: Building
  libzip failed: not all outputs were created or valid`).

Per the plan's documented fallback, both libzip and zlib are therefore
**vendored** under `third_party/` with the same native `cc_library` overlays
rules_celix uses (`third_party/libzip/libzip.BUILD`,
`third_party/zlib/zlib.BUILD`). The config headers for libzip (`config.h`,
`zipconf.h`, the CMake-generated `zip_err_str.c`) are vendored. This keeps the
module hermetic (no host cmake, no foreign cc, no boringssl/zstd) and
byte-compatible with the rules_celix semantic baseline.

Consequence: if a future BCR `libzip` gains a native-build overlay that ends up
in `lib/`, the module can switch back to `bazel_dep(name = "libzip")` with one
`framework/BUILD.bazel` edit (deps label back to `@libzip//:libzip`).

## Framework link-mode split (rules_celix issue #13) preserved

- `:framework` = headers-only `cc_library` (no `srcs`), for `framework =
  "runtime"` activator bundles. Verified: building it produces no compiled
  objects.
- `:framework_static` = full archive `cc_library` with `alwayslink = True`,
  for `framework = "static"` bundles and container runners. Verified: identical
  output shape (`libframework_static.lo`/`.so`) vs rules_celix's own build.
- `:utils` / `:utils_headers` mirror the rules_celix split; `:utils` links
  against the vendored libzip.

## Validation performed

- `bazel build //...` and `bazel test //tests:framework_smoke_test` in this
  repo: green.
- Cross-repo: a temporary rules_celix checkout pointing at this repo via
  `bazel_dep(name = "celix")` + `local_path_override` (and dropping `celix`
  from its `celix_deps` use_repo) builds `//examples/hello_container`, boots it
  (C + C++ activators start/stop in the runner), and passes the full
  `bazel test //...` suite (60/60 tests) on Linux.

## Version pinning

- Module version `2.4.0`, `compatibility_level = 1` (bump only on breaking
  framework ABI changes).
- Bazel pinned via `.bazelversion` (7.7.0, matching rules_celix) + the `bazelw`
  launcher.
- Once published to the BCR, consumers use `bazel_dep(name = "celix",
  version = "2.4.0")`; until then `local_path_override` / `git_override`.

## Post-review hardening (2026-11-25)

- **rules_cc 0.0.9 -> 0.1.5.** 0.0.9 registers its C++ toolchain via
  `use_extension("@bazel_tools//tools/cpp:cc_configure.bzl",
  "cc_configure_extension")`. Bazel 8 replaced that file with a redirect that
  exports only `cc_configure` and `MSVC_ENVVARS`, so module evaluation failed
  on 8.x with "extension not found". rules_cc 0.1.0+ defines the extension in
  `@rules_cc//cc:extensions.bzl` (the Bazel 8 path). The BCR presubmit matrix
  runs Bazel 7.x and 8.x, so 8.x support is mandatory. 0.1.5 is the latest
  0.1.x and stays lightweight; 0.2.x is Bazel 8-only and drags in
  protobuf/apple_support, so it is not used. Verified on Bazel **7.7.0**
  (`.bazelversion` dev default) and **8.9.0** (latest stable 8.x at time of
  writing): `bazel build //...` + `bazel test //...` green on both majors.
  `MODULE.bazel.lock` was regenerated; `bazel_features`/`bazel_skylib` appear
  as new transitive entries.
- **Bazel 8 leg in CI.** `.github/workflows/ci.yml` runs the build/test matrix
  over os x bazel (`7.x`, `8.x`), mirroring the `.bcr/presubmit.yml` matrix.
- **Release workflow (`.github/workflows/release.yml`).** A `v*` tag push
  builds the exact asset `.bcr/source.template.json` promises:
  `celix-v{TAG}.tar.gz` via `git archive --prefix="celix-{VERSION}/"`, unpacks
  to a single top-level `celix-{VERSION}/` directory, records sha256 + SRI
  integrity on the job summary, then verifies the exact shipped bytes build
  standalone on Bazel 7.x and 8.x (fresh `--output_base`, extract to a temp
  dir) before publishing the GitHub release (`gh release create`). Layout and
  tag/version consistency are checked inside the workflow; ci.yml's `release`
  gate remains the cheap tree-health veto on tags.
- **`-ldl` scoped to Linux.** `_FRAMEWORK_LINKOPTS` is now
  `select({linux: [-ldl, -lpthread], macos: [-lpthread], default: []})`;
  previously the default branch applied `-ldl` to every non-macOS platform,
  which does not link on Windows (a listed target per `bazelw.bat`).
- **Smoke test exercises the framework.** `tests/smoke_main.c` now calls
  `celix_frameworkFactory_createFramework(NULL)` and
  `celix_frameworkFactory_destroyFramework(fw)` instead of only printing; this
  covers start/stop plus dlopen/thread teardown without needing a cache-dir
  property (the factory takes ownership of a nullable `config`).
- **gen/ header drift check.** CI compares `framework/gen/` to the vendored
  include-tree copies (`celix_framework_export.h`, `celix_utils_export.h`,
  `celix_err_constants.h`) and fails on any diff.

## Open questions

- Whether upstream Apache Celix prefers the vendored tree (Option 1, adopted
  here) or the `http_archive` fetch (Option 2). If Option 2, the `gen/` headers
  become a patch applied to the archive and `framework/libs/` is removed.
- Upstream comfort hosting the `uuid/` stub (no BCR `uuid` module exists).