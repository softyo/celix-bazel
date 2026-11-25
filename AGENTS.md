# AGENTS.md

Guidance for AI agents working in this repository. This is a Bazel module that
packages Apache Celix 2.4.0 as a dependency-manageable module for consumption
by rules_celix and other bzlmod workspaces.

## Project purpose

- Vendors a trimmed snapshot of the Apache Celix 2.4.0 source tree (framework
  core + `libs/utils`) under `framework/libs/`.
- Exposes it as native `cc_library` targets so consumers can link the Celix
  framework into Bazel builds. No CMake, no system Celix, no system libuuid.
- Standalone project; independent of rules_celix (which consumes this module
  as its canonical framework). See `docs/IMPLEMENTATION_NOTES.md` for the
  full port history and open questions.

## Build system

- bzlmod-first. `MODULE.bazel` is the authoritative definition
  (`module(name = "celix", version = "2.4.0", compatibility_level = 1)`);
  `WORKSPACE` is a stub only.
- Bazel is pinned to 7.7.0 via `.bazelversion`. Use the `./bazelw` launcher
  (bootstraps bazelisk into `~/.cache/bazelisk`); the system has no
  `bazel`/`bazelisk` on PATH.
- `.bazelrc` sets a dot-prefixed symlink prefix (`.bazel-bin`, `.bazel-out`,
  ...), matching `.gitignore`. Do not add a plain `bazel-*` symlink setup.
- Module deps: only `rules_cc` and `platforms`. Framework hard deps (zlib,
  libzip) are vendored, not BCR deps — do not reintroduce BCR `libzip` (it
  builds via `rules_foreign_cc`/CMake, requires host cmake, and fails to
  produce the declared `lib/libzip.a` output on this host).

## Commands

```sh
./bazelw build //...             # full build
./bazelw build //framework:framework
./bazelw build //framework:framework_static
./bazelw build //framework:utils
./bazelw test //tests:framework_smoke_test
```

`//tests:framework_smoke_test` is the only test; it links
`//framework:framework_static` into a tiny C executable and creates, starts,
and destroys a framework (lifecycle + link-mode coverage). Starlark formatting
is enforced by the `buildifier` check job in `ci.yml`
(`@buildifier_prebuilt//:buildifier -- --mode=check -r .`); CI also runs the
build/test matrix on Bazel 7.x and 8.x (mirroring the BCR presubmit matrix) and
checks the `framework/gen/` headers against their vendored include copies.

## Repository layout

```
BUILD.bazel                  # root aliases only (@celix//:framework, :framework_static, :utils)
MODULE.bazel                 # module def + deps (bzlmod-first)
WORKSPACE                    # inert stub
framework/BUILD.bazel        # real framework + utils cc_library targets
framework/libs/              # vendored Celix 2.4.0 snapshot (framework + utils)
framework/gen/               # checked-in CMake-generated headers
third_party/zlib/            # zlib 1.3.1 vendored, native overlay
third_party/libzip/          # libzip 1.10.1 vendored, native overlay
uuid/                        # hermetic RFC 4122 libuuid replacement
tests/                       # smoke test only
docs/IMPLEMENTATION_NOTES.md # decisions history — read before changing the build
```

## Public API (must not break)

Root package aliases into the real targets; consumers reference `@celix//:...`:

- `@celix//:framework` — headers-only (`no srcs`) framework + utils include
  tree. Default for bundle activators (`framework = "runtime"`).
- `@celix//:framework_static` — full static archive, `alwayslink = True`
  (linux `linkopts: -ldl -lpthread`, macos `-lpthread`).
- `@celix//:utils` — compiled `libs/utils` archive, `alwayslink = True`, deps
  on vendored libzip.
- `@celix//uuid:uuid` — RFC 4122 stub (`copts = ["-std=c11"]`).

## Critical invariants (do not break)

- **Single-instance guarantee / link-mode split.** A process hosts only one
  Celix framework. `:framework` must stay headers-only: a compiled copy would
  embed a second framework instance into an activator and ODR-crash C++
  activators (`celix::impl::createActivator`). Activators resolve `celix_*`
  against the runner's embedded archive at dlopen time.
- `:framework_static` and `:utils` need `alwayslink = True` so runner archives
  export the complete `celix_*` surface for dlopened bundles.
- Dependency chain: `:framework_static` -> `:utils` + `//uuid`;
  `:utils` -> `//third_party/libzip`; libzip -> zlib.
- `celix_launcher.c` is intentionally NOT compiled and the `:framework_static`
  and `:utils` `srcs` are explicit ground-truth lists, not globs. New source
  files from upstream must be added deliberately and checked for transitive
  deps (e.g. launcher pulls in libcurl).
- `libs/utils/src/memstream/` exists but `fmemopen.c`/`open_memstream.c` are
  not in the utils `srcs` list. Do not silently add them.

## Platform conditional conventions

- Use `select` on `@platforms//os:*` for OS-specific flags; existing cases:
  - `framework` (_FRAMEWORK_LINKOPTS): linux `-ldl -lpthread`, macos
    `-lpthread`, other platforms none (`-ldl` does not exist on Windows;
    `bazelw.bat` targets Windows).
  - `framework`/`utils` (_LINUX_GNU_SOURCE_COPTS): `-D_GNU_SOURCE` on linux
    only, matching upstream Celix's CMake build. libc needs it to declare the
    GNU extensions the vendored sources use (`qsort_r` in array_list.c,
    `asprintf`/`strcasestr` in celix_framework_utils.c/service_tracker.c, and
    the `program_invocation_short_name`/`pthread_setname_np` guarded paths);
    without it, newer compilers (GCC 14+) fail on implicit declarations.
  - zlib: `-DZ_HAVE_UNISTD_H` on linux + macos.
  - libzip: `-D_FILE_OFFSET_BITS=64` on macos only.
- libzip's `zip_*.c` glob must keep excluding the crypto (openssl/gnutls/
  mbedtls/commoncrypto), compression (bzip2/xz/zstd), winzip-auth, and
  win32-only subsystems. Adding any excluded source drags new deps.

## Versioning / upstream

- Bump the module `version` (and `compatibility_level`, only on breaking ABI
  changes) in `MODULE.bazel`; keep README and `bazelw` state consistent.
- If upstream switches to Option 2 (http_archive), `framework/gen/` becomes a
  patch and `framework/libs/` is removed — see open questions in
  `docs/IMPLEMENTATION_NOTES.md` before choosing.