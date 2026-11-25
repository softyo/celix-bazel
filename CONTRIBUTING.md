# Contributing to celix-bazel

Thank you for your interest in contributing to **celix-bazel**, the Bazel module that packages Apache Celix as a dependency-manageable unit.
We welcome bug reports, feature requests, documentation improvements, and code patches.

## Getting Started

1. **Read the [README.md](README.md)** to understand the project's goals, the framework link-mode split, and how consumers use the module.
2. **Set up your environment**:
   - No pre-installed Bazel is required.
     Use the provided `./bazelw` (Linux/macOS) or `bazelw.bat` (Windows) wrapper scripts, which automatically download and use the correct version of Bazelisk/Bazel.
   - You need a working C/C++ toolchain (e.g. `gcc` or `clang`).
     The framework's hard dependencies (`zlib`, `libzip`) are vendored under `third_party/` with native Bazel overlays, so no CMake or system libraries are required.

## Reporting Bugs and Requesting Features

When you click **New Issue** on our GitHub repository, you will be presented with two structured templates:

- **🐛 Bug Report**: For unexpected behavior, errors, or build failures.
- **✨ Enhancement Request**: For new features or improvement suggestions.

Please fill out the form as completely as possible.
For bug reports, providing your Bazel version, OS, and minimal reproduction steps is very helpful.
Before submitting, check existing issues to avoid duplicates.

## Development Workflow

### Formatting

All Starlark (`.bzl`) files must be formatted with **buildifier**.
The repository includes a `buildifier_prebuilt` dev dependency.

To check the entire repository:

```bash
./bazelw build @buildifier_prebuilt//:buildifier
./bazelw run @buildifier_prebuilt//:buildifier -- --mode=check -r .
```

To format in place:

```bash
./bazelw run @buildifier_prebuilt//:buildifier -- -r .
```

### Building and Testing

Build the full module:

```bash
./bazelw build //...
```

Run the test suite (the smoke test links `//framework:framework_static`):

```bash
./bazelw test //tests:framework_smoke_test
```

### Pre-Submission Checklist

Before opening a Pull Request, please ensure:

- [ ] All Starlark files are formatted with `buildifier`.
- [ ] `./bazelw build //...` passes.
- [ ] `./bazelw test //tests:framework_smoke_test` passes.
- [ ] All commits are signed off (`git commit -s`).
      CI checks this for you.
- [ ] If you changed public targets or behavior, the documentation and README are updated as appropriate.
- [ ] If you added or changed vendored sources, the explicit `srcs` lists in `framework/BUILD.bazel` are updated deliberately, and transitive dependencies are accounted for.
- [ ] Your commit messages are clear and reference any related issues.

## Code Style

- Follow the official [Starlark Style Guide](https://github.com/bazelbuild/starlark-style-guide).
- Add docstrings to public rules, macros, and providers.
- Keep `MODULE.bazel`, `MODULE.bazel.lock`, and `.bazelversion` consistent with the module version.
- When editing the build, read `docs/IMPLEMENTATION_NOTES.md` first — it records the link-mode split and the vendored zlib/libzip decisions that must not be silently reverted.

## Pull Request Guidelines

- Keep PRs focused and reasonably sized.
- Link to any related GitHub Issues.
- The module is currently **pre-1.0**.
  While we aim for API stability, minor breaking changes may occur before v1.0.
  Clearly note any breaking changes in your PR description.

## Licensing & Contributions

### Apache 2.0 License

All code in this repository is licensed under the [Apache License, Version 2.0](LICENSE).
The vendored source is Apache Celix, which is also Apache-2.0; see `NOTICE`.

### Sign-Off Your Commits (DCO)

This standalone repository uses the Developer Certificate of Origin (DCO) process as the interim contributor step: by signing off your commits you certify that you have the right to submit your code under the Apache 2.0
license.
Sign commits with:

```bash
git commit -s -m "Your commit message"
```

This adds a `Signed-off-by:` trailer using your configured `user.name` and `user.email`.
Use your real name; no pseudonyms or anonymous contributions.

### Future Apache Adoption

This module may later move under the Apache Celix project.
Apache projects use the Individual Contributor License Agreement (ICLA) rather than DCO as the formal contribution mechanism.
If and when the module is handed over, the maintainers will communicate the ICLA process for contributors; please do not assume DCO sign-off alone satisfies Apache's contribution requirements.

If you are considering a large or structural change and anticipate a future Apache handoff, please open an issue first to coordinate.

Thank you for helping make `celix-bazel` better!
