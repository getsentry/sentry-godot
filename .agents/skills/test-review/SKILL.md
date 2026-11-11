---
name: test-review
description: Finds sentry-godot tests that cannot fail, pass for the wrong reason, race against asynchronous capture, or break on a supported platform. Use for pull request review of GDScript gdUnit4 tests, C++ doctest tests, Pester integration tests, web Playwright tests, .NET tests, Android JUnit tests, and the JS bridge test.
allowed-tools: Read Grep Glob
---

You review test changes for the Sentry SDK for Godot Engine. Report only tests
that would hide a real regression, fail on a platform CI runs, or call code that
does not exist. Missing coverage on its own is not a finding.

## Test Layout

| Path | Framework | Notes |
|------|-----------|-------|
| `project/test/suites/` | gdUnit4 | Run together in one Godot process per platform. |
| `project/test/isolated/` | gdUnit4 | One process per file, for tests that need their own `init_sdk()` or project settings. |
| `project/test/util/sentry_test_suite.gd` | helpers | `init_sdk`, `_before_send`, `wait_for_captured_event_json`, `assert_json`. |
| `project/test/dotnet/DotnetTestHarness.cs` | C# harness | Called by name from integration and C++ tests. |
| `tests/cpp/tests/` | doctest | Built into the extension with `tests=yes`. |
| `tests/integration/` | Pester | Runs exported apps on desktop, Android, iOS, and Sauce Labs; shared cases in `CommonTestCases.ps1`. |
| `tests/web/` | Playwright | Web export tests. |
| `android_lib/src/test/` | JUnit | Kotlin plugin unit tests, run by Gradle. |
| `src/sentry/javascript/bridge/test.js` | Node | JS bridge bundle test (`npm test`). |

Platform skips use `before(_do_skip = ...)` with `OS.has_feature()` or
`OS.get_name()`. Unit tests run on Godot 4.5, 4.6, and 4.7 across Linux,
Windows, and macOS, plus Android and Web jobs.

## What To Report

### Tests that cannot fail
- An assertion that compares against a value the code under test already has by
  default, so the test passes even when the feature is not wired. Example: a
  round-trip test that writes `false` into options whose default is `false`.
- A test that covers options or fields that do not cross the boundary it claims
  to test, such as deprecated no-op options in a native and managed round trip.
- Asserting "nothing was captured" right after an action, before an
  asynchronous capture could arrive. Cocoa and the .NET layer can capture on
  another thread, so absence checks need a wait or a flush.

### Wrong wiring
- A test calls a harness method, signal, or helper by a name that does not
  exist. Grep the harness or helper to confirm.
- A wait helper that returns immediately because a signal was already emitted
  earlier in the test, so the next assertion reads the previous event.
- A shared test case (`CommonTestCases.ps1`) whose expected counts or output no
  longer match a context that now captures more or fewer events.

### Platform coverage
- A skip removed or added so that a test now runs on a backend that does not
  support the feature yet, or stops running on one that does. Check the backend
  code under `src/sentry/<backend>/` before you decide.
- Paths, line endings, or process handling in a runner that fail on one of the
  CI operating systems.

## What Not To Report

- Missing tests or low coverage.
- Test style, naming, duplication, or helper refactors.
- PowerShell or GDScript language claims you cannot confirm from this repo's
  code. Example: Pester script blocks accept extra named arguments; that was a
  false positive here.
- Bugs in SDK code under `src/` (handled by `sdk-review`).
- Flakiness you cannot tie to a specific race or ordering in the code.

## Severity

- **high**: a test that would pass while a shipped feature is broken on a whole
  platform.
- **medium**: a test that fails or flakes on a supported CI platform, or checks a
  weaker condition than its name says.
- **low**: a narrow gap in an otherwise valid test.

## Finding Format

- Title: the test name and why it passes or fails wrongly.
- Description: one or two sentences on the regression it hides and the fix.
- Verification: the default, timing, or name that makes the assertion vacuous
  or broken, with the file that proves it.
