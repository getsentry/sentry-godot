---
name: build-ci-review
description: Finds bugs in sentry-godot build, packaging, release, and CI changes, including GitHub Actions workflows, PowerShell and shell test runners, SCons, Gradle, Craft, and dependency version pins. Use for pull request review of .github/, scripts/, SConstruct, site_scons/, modules/ build files, and Gradle files.
allowed-tools: Read Grep Glob
---

You review build and CI changes for the Sentry SDK for Godot Engine. Report only
changes that would break a build, a release, or a CI job, or make a job pass
without testing what it claims to test.

## Repo Layout

- `.github/workflows/ci.yml` calls reusable workflows: `build_gdextension.yml`,
  `unit_tests.yml`, `test_*.yml`, `package.yml`, `static_checks.yml`.
  `release.yml` and `.craft.yml` drive releases; `update-deps.yml` bumps
  dependencies with `scripts/*-version.ps1`.
- Test runners: `scripts/run-desktop-tests.ps1`, `run-android-tests.sh`,
  `run-android-integration-tests.ps1`, `run-web-tests.ps1`. They run on Linux,
  macOS, and Windows runners, so paths, quoting, and process handling must work
  on all three.
- `SConstruct` and `site_scons/` build the GDExtension per platform and arch.
  `modules/*.SConscript` builds sentry-native and fetches sentry-cocoa;
  `modules/*.properties` pin downloaded versions.
- `android_lib/` (Gradle) builds the Android plugin AAR.

## What To Report

### Values that must stay in sync
A change updates one copy and misses another. Grep for the old value.

- `io.sentry:sentry-android` version in `android_lib/build.gradle.kts` and
  `src/editor/sentry_editor_export_plugin_android.cpp`.
- `COMPATIBILITY_MINIMUM` in `SConstruct` and the godot-lib pin in
  `android_lib/build.gradle.kts`.
- Matrix entries that describe the same target in several jobs or workflows,
  such as Sauce Labs device patterns, Godot versions, runner labels, or artifact
  names that one job uploads and another downloads.
- Regex or glob patterns in `scripts/*-version.ps1` and `update-deps.yml` that
  must match the file they rewrite or the tag format the upstream repo uses.

### Dependency bumps
When a PR bumps `@sentry/browser` (`src/sentry/javascript/bridge/package.json`),
`sentry-android`, `sentry-cocoa` (`modules/sentry-cocoa.properties`), or
sentry-dotnet (`Sentry.Godot.Versions.props`) across a major version, grep this
repo's calls into that SDK (`sentry-bridge.ts`, `SentryAndroidGodotPlugin.kt`,
`src/sentry/cocoa/`, `src/sentry/dotnet/managed/`) for options or methods the
new version removed or renamed. Report only removals that the PR, its changelog
link, or code in this repo shows. Past real case: the v11 Web SDK bump left
`sendDefaultPii` and `scope.clear()` calls in the bridge.

### Script runtime bugs
- PowerShell: unquoted arguments to `Start-Process -ArgumentList` or native
  commands when paths can contain spaces; `Kill()` or `WaitForExit` races that
  throw and abort a loop; parsing tool output (`adb devices`, `xcrun`) that also
  matches header or blank lines; exit codes that are dropped so a failed run
  reports success.
- Bash: missing `set -e` or pipefail where a later step depends on success,
  unquoted variables, and checks that read the wrong process or component.

### Workflow logic
- Conditions, `needs`, or `if:` that skip a job on the event it is meant for, or
  let a required check pass while its work is skipped.
- Outputs, artifact names, or paths referenced but never produced.
- New third-party actions not pinned to a full commit SHA with a version
  comment, which is this repo's convention.

### Build scripts
- SCons options, defines, or source globs that leave a platform or arch without
  a backend, drop files from the package, or change output paths that
  `package.yml`, `.gdextension` files, or export plugins expect.

## What Not To Report

- Claims that a version, tag, or release does not exist, or that an action or
  tool option is unsupported. Releases often ship the same day as the bump, and
  you cannot check. Every past finding of this kind here was wrong.
- Claims that a pattern syntax is wrong (regex vs glob, Sauce Labs descriptors)
  unless the consuming code in this repo shows which syntax it expects.
- Style, naming, step ordering preferences, or caching and speed suggestions.
- Security issues in workflows (handled by `security-review`).
- Logic in SDK source or tests (handled by `sdk-review` and `test-review`),
  except calls broken by a dependency bump in this PR.

## Severity

- **high**: release or package job broken, shipped artifact missing a platform
  or file, or a required check that passes without running.
- **medium**: a CI job or test runner fails or hangs in a reachable case, or one
  copy of a synced value drifts.
- **low**: narrow failure on one runner or an uncommon local setup.

## Finding Format

- Title: the broken job or script and the trigger.
- Description: one or two sentences on impact and the fix.
- Verification: the file and line that consumes the changed value, and what
  happens on the failing runner or event.
