# BCR release runbook

How a `celix-bazel` release is cut and submitted to the Bazel Central Registry
(BCR). Short and copyable: run the release, grab the hashes, submit the PR.

## What the release workflow does

`.github/workflows/release.yml` runs automatically on a `v*` tag push. It:

1. Fails fast unless the tag matches the `MODULE.bazel` version.
2. Builds the source archive `git archive` ships on the tag:
   `celix-v{TAG}.tar.gz`, unpacking to a single top-level `celix-{VERSION}/`
   directory (this is exactly the contract of
   `.bcr/source.template.json`).
3. Prints the sha256 and SRI integrity in the workflow **job summary**.
4. Verifies the exact shipped bytes build standalone on Bazel 7.x and 8.x
   (fresh `--output_base`, extract to a temp dir) — the BCR presubmit matrix.
5. Publishes the GitHub release with the archive attached and generated notes.

So the release tag + the job summary give you everything the BCR submission
needs; the hermetic check runs before the release is created.

## Cutting a release

```sh
git tag -s v2.4.0            # annotated; keep signed tags for releases
git push origin v2.4.0       # triggers release.yml
```

Watch the run at `https://github.com/softyo/celix-bazel/actions`. When the
`build-source-archive` job finishes, its summary contains the exact
`integrity`, `strip_prefix`, and `url` values for the BCR.

## Submitting to the BCR (manual, per release)

The release workflow publishes the GitHub release; it does **not** submit the
BCR pull request. That step is manual:

1. Fork `bazelbuild/bazel-central-registry`.
2. Create `modules/celix/2.4.0/`:
   - `metadata.json` — filled from `.bcr/metadata.template.json`; set the single
     element of `"versions"` to `"2.4.0"`.
   - `source.json` — from the release job summary:
     ```json
     {
       "integrity": "sha256-<from job summary>",
       "strip_prefix": "celix-2.4.0",
       "url": "https://github.com/softyo/celix-bazel/releases/download/v2.4.0/celix-v2.4.0.tar.gz"
     }
     ```
   - `presubmit.yml` — copy `.bcr/presubmit.yml` verbatim.
   - `MODULE.bazel` — copy from the archive
     (`celix-2.4.0/MODULE.bazel`, i.e. the tagged tree's `MODULE.bazel`).
   - `LICENSE`, `NOTICE` — required by the BCR for the module. Copy from the
     archive root.
3. Open the PR against `bazelbuild/bazel-central-registry` with just these
   files. The BCR bot runs the presubmit (Bazel 7.x + 8.x from
   `presubmit.yml`) and adds the `metadata.json` checksums after approval.
4. Do not force-update `version` fields on the PR branch after the bot has
   commented; fix forward instead.

Hints:

- Commit the generated `MODULE.bazel.lock` alongside the release tag; the BCR
  source.json `integrity` is computed over the archive, which is `git
  archive` output — the lock file inside it is deterministic.
- Verify the BCR archive by hand once with
  `curl -L <url> | tar tz | head` and expect `celix-2.4.0/MODULE.bazel` first.
