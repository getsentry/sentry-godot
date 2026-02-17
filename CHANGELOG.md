# Changelog

## Unreleased

### Features

- Support Web platform ([#488](https://github.com/getsentry/sentry-godot/pull/488))

### Dependencies

- Bump Sentry Android from v8.29.0 to v8.32.0 ([#473](https://github.com/getsentry/sentry-godot/pull/473))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8320)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.29.0...8.32.0)
- Bump Native SDK from v0.12.3 to v0.12.8 ([#487](https://github.com/getsentry/sentry-godot/pull/487))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#0128)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.12.3...0.12.8)
- Bump Sentry JavaScript from v10.34.0 to v10.39.0 ([#527](https://github.com/getsentry/sentry-godot/pull/527))
  - [changelog](https://github.com/getsentry/sentry-javascript/blob/develop/CHANGELOG.md#10390)
  - [diff](https://github.com/getsentry/sentry-javascript/compare/10.34.0...10.39.0)

## 1.3.2

### Fixes

- Fix options pollution across `close()`/`init()` cycles by recreating `SentryOptions` on each initialization ([#503](https://github.com/getsentry/sentry-godot/pull/503))
- Fix warning about instantiated `SentryLogger` used as default value on startup ([#505](https://github.com/getsentry/sentry-godot/pull/505))

### Changes

- The `enable_logs` option is now turned off by default to align with other Sentry SDKs. If you want to keep it enabled, go to **Sentry > Options > Enable Logs** in the **Project Settings** and turn it on. ([#484](https://github.com/getsentry/sentry-godot/pull/484))

## 1.3.1

### Fixes

- Fix crash when sending crash reports from previous session in exported iOS projects ([#475](https://github.com/getsentry/sentry-godot/pull/475))

### Dependencies

- Bump Native SDK from v0.12.2 to v0.12.3 ([#470](https://github.com/getsentry/sentry-godot/pull/470))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#0123)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.12.2...0.12.3)

## 1.3.0

### Features

- Add `environment` option to project settings ([#469](https://github.com/getsentry/sentry-godot/pull/469))

### Improvements

- Switched from shipping frameworks to dylibs on macOS to avoid Windows symlink issues and prevent TestFlight rejections caused by malformed framework bundles ([#468](https://github.com/getsentry/sentry-godot/pull/468))

### Dependencies

- Bump Sentry Android from v8.28.0 to v8.29.0 ([#465](https://github.com/getsentry/sentry-godot/pull/465))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8290)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.28.0...8.29.0)

## 1.2.0

### Features

- Structured Logs are now generally available and enabled by default ([#460](https://github.com/getsentry/sentry-godot/pull/460))
- String interpolation and custom attributes support in structured logging API ([#435](https://github.com/getsentry/sentry-godot/pull/435))

### Fixes

- Fix User Feedback UI text cursor visibility in default theme ([#445](https://github.com/getsentry/sentry-godot/pull/445))
- Fix custom tags missing in native crashes on Android ([#454](https://github.com/getsentry/sentry-godot/pull/454))

### Dependencies

- Bump Sentry Android from v8.25.0 to v8.28.0 ([#444](https://github.com/getsentry/sentry-godot/pull/444), [#452](https://github.com/getsentry/sentry-godot/pull/452), [#458](https://github.com/getsentry/sentry-godot/pull/458))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8280)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.25.0...8.28.0)
- Bump Cocoa SDK from v8.57.2 to v8.57.3 ([#448](https://github.com/getsentry/sentry-godot/pull/448))
  - [changelog](https://github.com/getsentry/sentry-cocoa/blob/main/CHANGELOG.md#8573)
  - [diff](https://github.com/getsentry/sentry-cocoa/compare/8.57.2...8.57.3)
- Bump Native SDK from v0.12.1 to v0.12.2 ([#455](https://github.com/getsentry/sentry-godot/pull/455))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#0122)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.12.1...0.12.2)

## 1.1.1

### Fixes

- Fixed SentryLog.set_attribute() not working on Android ([#438](https://github.com/getsentry/sentry-godot/pull/438))
- Fixed occasional crash on exit on Android ([#439](https://github.com/getsentry/sentry-godot/pull/439))
- Initialize attributes dict in CocoaSDK::log unconditionally ([#440](https://github.com/getsentry/sentry-godot/pull/440))

### Dependencies

- Bump Cocoa SDK from v8.57.0 to v8.57.2 ([#432](https://github.com/getsentry/sentry-godot/pull/432))
  - [changelog](https://github.com/getsentry/sentry-cocoa/blob/main/CHANGELOG.md#8572)
  - [diff](https://github.com/getsentry/sentry-cocoa/compare/8.57.0...8.57.2)
- Bump Native SDK from v0.11.3 to v0.12.1 ([#431](https://github.com/getsentry/sentry-godot/pull/431))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#0121)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.11.3...0.12.1)
- Bump Sentry Android from v8.24.0 to v8.25.0 ([#430](https://github.com/getsentry/sentry-godot/pull/430))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8250)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.24.0...8.25.0)

## 1.1.0

### Features

- Add user feedback API for collecting and sending user feedback to Sentry ([#418](https://github.com/getsentry/sentry-godot/pull/418))
- Add customizable User Feedback form that can be used for feedback submission, and as an example for custom implementations ([#422](https://github.com/getsentry/sentry-godot/pull/422))
- Access event exception values in `before_send` handler ([#415](https://github.com/getsentry/sentry-godot/pull/415))
- Add support for Structured Logging ([#409](https://github.com/getsentry/sentry-godot/pull/409))

### Improvements

- Detect when we're inside message logging to prevent SDK print operations through the Godot logger which cause runtime errors. ([#414](https://github.com/getsentry/sentry-godot/pull/414))
- Relax throttling limits on app startup ([#423](https://github.com/getsentry/sentry-godot/pull/423))
- Set app hang timeout to 5s on Apple platforms ([#416](https://github.com/getsentry/sentry-godot/pull/416))
- Add app hang tracking options and disable this feature by default ([#429](https://github.com/getsentry/sentry-godot/pull/429))

### Dependencies

- Bump Native SDK from v0.11.2 to v0.11.3 ([#420](https://github.com/getsentry/sentry-godot/pull/420))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#0113)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.11.2...0.11.3)
- Bump Cocoa SDK from v8.56.2 to v8.57.0 ([#419](https://github.com/getsentry/sentry-godot/pull/419))
  - [changelog](https://github.com/getsentry/sentry-cocoa/blob/main/CHANGELOG.md#8570)
  - [diff](https://github.com/getsentry/sentry-cocoa/compare/8.56.2...8.57.0)
- Bump Sentry Android from v8.23.0 to v8.24.0 ([#424](https://github.com/getsentry/sentry-godot/pull/424))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8240)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.23.0...8.24.0)

## 1.0.0

### Breaking changes

- The `attach_screenshot` and `screenshot_level` options have moved to the experimental section while we're still improving things. If you previously had it enabled, you will need to re-enable it in its new location. They're currently not recommended for production use. ([#375](https://github.com/getsentry/sentry-godot/pull/375))
- Remove `SentrySDK.get_user()` from API and ensure `user.ip_address` defaults to auto when `send_default_pii` is ON ([#392](https://github.com/getsentry/sentry-godot/pull/392))

### Improvements

- Improve scene tree data capture performance ([#373](https://github.com/getsentry/sentry-godot/pull/373))
- Set device.name to OS hostname on Windows/Linux dedicated servers ([#391](https://github.com/getsentry/sentry-godot/pull/391))
- Prevent usage of Godot logger during crash handling on Windows/Linux ([#398](https://github.com/getsentry/sentry-godot/pull/398))
- Add missing Cocoa SDK symbols to builds ([#401](https://github.com/getsentry/sentry-godot/pull/401))
- Add build option to separate debug symbols for GDExtension and crashpad_handler, and do it in the official builds ([#399](https://github.com/getsentry/sentry-godot/pull/399))
- Support separating debug symbols of Android targets ([#404](https://github.com/getsentry/sentry-godot/pull/404))
- Generate Info.plist for macOS during build ([#403](https://github.com/getsentry/sentry-godot/pull/403))

### Fixes

- Fixed setting `throttle_window_ms` to 0 should disable it ([#382](https://github.com/getsentry/sentry-godot/pull/382))
- Fixed failing to set initial user on Apple platforms ([#390](https://github.com/getsentry/sentry-godot/pull/390))
- Added missing `crashpad_wer.dll` to Windows builds and export dependencies ([#396](https://github.com/getsentry/sentry-godot/pull/396))
- Sanitize Variant values on Android, and fix custom context and local variables missing or null on Android ([#397](https://github.com/getsentry/sentry-godot/pull/397))

### Other changes

- Demo: Add "Run tests" button instead of running tests on start on mobile platforms ([#379](https://github.com/getsentry/sentry-godot/pull/379))
- Add "build_android_lib" option to SConstruct ([#384](https://github.com/getsentry/sentry-godot/pull/384))

### Dependencies

- Bump Cocoa SDK from v8.56.0 to v8.56.2 ([#383](https://github.com/getsentry/sentry-godot/pull/383), [#393](https://github.com/getsentry/sentry-godot/pull/393))
  - [changelog](https://github.com/getsentry/sentry-cocoa/blob/main/CHANGELOG.md#8562)
  - [diff](https://github.com/getsentry/sentry-cocoa/compare/8.56.0...8.56.2)

- Bump Sentry Android from v8.21.1 to v8.23.0 ([#380](https://github.com/getsentry/sentry-godot/pull/380), [#402](https://github.com/getsentry/sentry-godot/pull/402))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8230)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.21.1...8.23.0)

- Bump Native SDK from v0.10.1 to v0.11.2 ([#374](https://github.com/getsentry/sentry-godot/pull/374), [#385](https://github.com/getsentry/sentry-godot/pull/385), [#405](https://github.com/getsentry/sentry-godot/pull/405))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#0112)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.10.1...0.11.2)

## 1.0.0-beta.3

### Breaking changes

- Bumped the minimum Godot compatibility to `4.5-stable`, locking it in for the 1.x series. ([#369](https://github.com/getsentry/sentry-godot/pull/369))

### Improvements

- Strip invisible characters from logger breadcrumbs ([#359](https://github.com/getsentry/sentry-godot/pull/359))

### Dependencies

- Bump Cocoa SDK from v8.55.1 to v8.56.0 ([#361](https://github.com/getsentry/sentry-godot/pull/361))
  - [changelog](https://github.com/getsentry/sentry-cocoa/blob/main/CHANGELOG.md#8560)
  - [diff](https://github.com/getsentry/sentry-cocoa/compare/8.55.1...8.56.0)

## 1.0.0-beta.2

### Breaking changes

- Configuration script support and `SentryConfiguration` class are removed. Instead, please use manual initialization with a configuration callback, if you need to set up SDK from code. See [#321](https://github.com/getsentry/sentry-godot/pull/321) for details.
- `enabled` option is renamed to `auto_init` for clarity, and removed from SentryOptions properties (setting it from code has no sense - we auto-initialize very early).
- `disabled_in_editor_play` option is renamed to `skip_auto_init_on_editor_play` for clarity, and removed from SentryOptions properties.
- We bumped Godot compatibility to 4.5-rc2.

### Features

- Support local variables on Android ([#334](https://github.com/getsentry/sentry-godot/pull/334))
- Allow initializing manually and shutting down SentrySDK ([#321](https://github.com/getsentry/sentry-godot/pull/321))

### Other changes

- Use threads interface for error reporting with Native SDK ([#350](https://github.com/getsentry/sentry-godot/pull/350))

### Dependencies

- Bump Sentry Android from v8.20.0 to v8.21.0 ([#352](https://github.com/getsentry/sentry-godot/pull/352))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8210)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.20.0...8.21.0)
- Bump Sentry Android from v8.21.0 to v8.21.1 ([#353](https://github.com/getsentry/sentry-godot/pull/353))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8211)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.21.0...8.21.1)
- Bump Godot compatibility to 4.5-rc2 ([#356](https://github.com/getsentry/sentry-godot/pull/356))

## 1.0.0-beta.1

### Breaking changes

First, we bumped Godot compatibility to 4.5-beta7 ([#348](https://github.com/getsentry/sentry-godot/pull/348))

Second, we've redesigned the breadcrumb API for a cleaner, more intuitive interface. Previously, `add_breadcrumb()` method accepted 5 parameters (3 of which were strings), making it confusing to use. The new approach uses a dedicated `SentryBreadcrumb` class:

```gdscript
var crumb := SentryBreadcrumb.create("Something happened")
crumb.type = "info"
crumb.set_data({"some": "data"})
SentrySDK.add_breadcrumb(crumb)
```

For simple breadcrumbs, you can use a one-liner:
```gdscript
SentrySDK.add_breadcrumb(SentryBreadcrumb.create("Something happened"))
```

This change provides better type safety, improved readability, and enables future support for the `before_breadcrumb` callback.

### Features

- Add support for script context and variables on Apple platforms ([#306](https://github.com/getsentry/sentry-godot/pull/306))
- Add SentryEvent.to_json() ([#329](https://github.com/getsentry/sentry-godot/pull/329))

### Improvements

- Improve initialization flow ([#322](https://github.com/getsentry/sentry-godot/pull/322))
- Introduce `SentryBreadcrumb` class ([#332](https://github.com/getsentry/sentry-godot/pull/332))

### Fixes

- Potential crash in SentryLogger if removed early ([#323](https://github.com/getsentry/sentry-godot/pull/323))
- Ensure compatibility with minSdk 24 on Android ([#324](https://github.com/getsentry/sentry-godot/pull/324))
- Fixed UTF-8 retention problems with native SentryEvent properties ([#345](https://github.com/getsentry/sentry-godot/pull/345))

### Other changes

- Move native and Android internal code into respective namespaces ([#333](https://github.com/getsentry/sentry-godot/pull/333))

### Dependencies

- Bump Cocoa SDK from v8.54.0 to v8.55.0 ([#318](https://github.com/getsentry/sentry-godot/pull/318))
  - [changelog](https://github.com/getsentry/sentry-cocoa/blob/main/CHANGELOG.md#8550)
  - [diff](https://github.com/getsentry/sentry-cocoa/compare/8.54.0...8.55.0)
- Bump Cocoa SDK from v8.55.0 to v8.55.1 ([#349](https://github.com/getsentry/sentry-godot/pull/349))
  - [changelog](https://github.com/getsentry/sentry-cocoa/blob/main/CHANGELOG.md#8551)
  - [diff](https://github.com/getsentry/sentry-cocoa/compare/8.55.0...8.55.1)
- Bump Native SDK from v0.10.0 to v0.10.1 ([#344](https://github.com/getsentry/sentry-godot/pull/344))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#0101)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.10.0...0.10.1)
- Bump Sentry Android from v8.19.1 to v8.20.0 ([#325](https://github.com/getsentry/sentry-godot/pull/325))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8200)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.19.1...8.20.0)
- Bump gdUnit 4 from v5.1.0-8-g4357f3f to v5.1.0 ([#319](https://github.com/getsentry/sentry-godot/pull/319))
  - [changelog](https://github.com/MikeSchulze/gdUnit4/blob/master/CHANGELOG.md#v510)
  - [diff](https://github.com/MikeSchulze/gdUnit4/compare/v5.1.0-8-g4357f3f...v5.1.0)

## 1.0.0-beta.0

### Breaking changes

- Bump Godot compatibility to 4.5-beta5 ([#307](https://github.com/getsentry/sentry-godot/pull/307))

### Features

- Intoduce `SentryTimestamp` class to improve timestamp handling ([#286](https://github.com/getsentry/sentry-godot/pull/286))
- Support for iOS & macOS using Sentry Cocoa SDK integration ([#266](https://github.com/getsentry/sentry-godot/pull/266))
- Add option to disable logging messages as breadcrumbs ([#305](https://github.com/getsentry/sentry-godot/pull/305))

### Improvements

- Make error throttling smarter by factoring in error message ([#287](https://github.com/getsentry/sentry-godot/pull/287))

### Other changes

- Add iOS framework as optional build target ([#290](https://github.com/getsentry/sentry-godot/pull/290))
- Add demo icon ([#302](https://github.com/getsentry/sentry-godot/pull/302))

### Dependencies

- Bump Cocoa SDK from v8.53.2 to v8.54.0 ([#304](https://github.com/getsentry/sentry-godot/pull/304))
  - [changelog](https://github.com/getsentry/sentry-cocoa/blob/main/CHANGELOG.md#8540)
  - [diff](https://github.com/getsentry/sentry-cocoa/compare/8.53.2...8.54.0)
- Bump Native SDK from v0.9.1 to v0.10.0 ([#311](https://github.com/getsentry/sentry-godot/pull/311))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#0100)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.9.1...0.10.0)
- Bump Sentry Android from v8.18.0 to v8.19.1 ([#312](https://github.com/getsentry/sentry-godot/pull/312), [#315](https://github.com/getsentry/sentry-godot/pull/315))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8191)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.18.0...8.19.1)

## 1.0.0-alpha.3

### Breaking changes

- Remove `disabled_in_editor` option in favor of disabling SDK in the Godot editor by default.([#277](https://github.com/getsentry/sentry-godot/pull/277))
- Bump Godot minimum version to `4.5-beta4` ([#285](https://github.com/getsentry/sentry-godot/pull/285))

### Fixes

- Prevent feedback loops in SentryLogger ([#275](https://github.com/getsentry/sentry-godot/pull/275))
- Fix `release` option not honored if set to a custom value in the project settings, and fix parsing of `app_name`/`app_version` replacement tokens if set via a configuration script. ([#276](https://github.com/getsentry/sentry-godot/pull/276))
- Remove `libatomic.so.1` dependency on Linux ([#278](https://github.com/getsentry/sentry-godot/pull/278))
- Prevent stack overflow in variant conversion ([#284](https://github.com/getsentry/sentry-godot/pull/284))

### Dependencies

- Bump Sentry Android from v8.17.0 to v8.18.0 ([#282](https://github.com/getsentry/sentry-godot/pull/282))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8180)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.17.0...8.18.0)

## 1.0.0-alpha.2

### Features

- Auto-enable required project settings ([#260](https://github.com/getsentry/sentry-godot/pull/260))
- Add `disabled_in_editor_play` option ([#262](https://github.com/getsentry/sentry-godot/pull/262))

### Fixes

- Allow configuration script to run even if SDK is initially disabled in project settings ([#258](https://github.com/getsentry/sentry-godot/pull/258))
- Fix runtime errors with logger connecting to signal and early initialization ([#265](https://github.com/getsentry/sentry-godot/pull/265))
- Fix context-related errors generated at runtime with early intialization ([#264](https://github.com/getsentry/sentry-godot/pull/264))

## 1.0.0-alpha.1

### Fixes

- Check options.enabled during SDK initialization ([#250](https://github.com/getsentry/sentry-godot/pull/250))
- Fix crashing when SDK is disabled via options ([#253](https://github.com/getsentry/sentry-godot/pull/253))

## 1.0.0-alpha.0

### Breaking changes

- Minimum Godot version required is now 4.5, which is needed for the Logger interface ([#201](https://github.com/getsentry/sentry-godot/pull/201)) and Android support ([#169](https://github.com/getsentry/sentry-godot/pull/169))

### Features

- Initial Android support ([#169](https://github.com/getsentry/sentry-godot/pull/169))
- Refine demo for mobile screens ([#196](https://github.com/getsentry/sentry-godot/pull/196))
- Add user attachments support ([#205](https://github.com/getsentry/sentry-godot/pull/205))
- Provide x86_32 builds for Windows and Linux ([#218](https://github.com/getsentry/sentry-godot/pull/218))
- Support for the new Logger interface (more accurate error reporting), script stack traces and local variables in Godot 4.5 ([#201](https://github.com/getsentry/sentry-godot/pull/201))
- Provide noop (stub) builds for unsupported platforms ([#239](https://github.com/getsentry/sentry-godot/pull/239))

### Fixes

- Fixed Godot 4.5 complaining that "usage" is not supported ([#214](https://github.com/getsentry/sentry-godot/pull/214))

### Other changes

- Filter SDK messages in logger ([#233](https://github.com/getsentry/sentry-godot/pull/233))

### Dependencies

- Bump Native SDK from v0.9.0 to v0.9.1 ([#210](https://github.com/getsentry/sentry-godot/pull/210))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#091)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.9.0...0.9.1)
- Bump Sentry Android from v8.14.0 to v8.17.0 ([#212](https://github.com/getsentry/sentry-godot/pull/212), [#242](https://github.com/getsentry/sentry-godot/pull/242))
  - [changelog](https://github.com/getsentry/sentry-java/blob/main/CHANGELOG.md#8170)
  - [diff](https://github.com/getsentry/sentry-java/compare/8.14.0...8.17.0)
- Bump gdUnit 4 from v5.0.4 to v5.0.5 ([#221](https://github.com/getsentry/sentry-godot/pull/221))
  - [changelog](https://github.com/MikeSchulze/gdUnit4/blob/master/CHANGELOG.md#v505)
  - [diff](https://github.com/MikeSchulze/gdUnit4/compare/v5.0.4...v5.0.5)
- Bump godot-cpp to sync with Godot 4.5-beta3 ([#246](https://github.com/getsentry/sentry-godot/pull/246))

## 0.6.1

### Fixes

- Ensure crashpad_handler has executable permissions on Unix exports ([#207](https://github.com/getsentry/sentry-godot/pull/207))

### Dependencies

- Bump gdUnit 4 from v5.0.0 to v5.0.4 ([#193](https://github.com/getsentry/sentry-godot/pull/193), [#197](https://github.com/getsentry/sentry-godot/pull/197))
  - [changelog](https://github.com/MikeSchulze/gdUnit4/blob/master/CHANGELOG.md#v504)
  - [diff](https://github.com/MikeSchulze/gdUnit4/compare/v5.0.0...v5.0.4)

## 0.6.0

### Breaking changes

- Remove `on_crash` hook in favor of `SentryEvent.is_crash()`: `before_send` is now called for crash events too, and you can check if it's a crash event by calling `event.is_crash()` ([#181](https://github.com/getsentry/sentry-godot/pull/181))

### Fixes

- Filter out warnings about missing attachment files that may not exist in some scenarios (screenshot.jpg and view-hierarchy.json) ([#189](https://github.com/getsentry/sentry-godot/pull/189))
- Fix crash in code editor when `disabled_in_editor` is ON ([#191](https://github.com/getsentry/sentry-godot/pull/191))

### Other changes

- Rename "addons/sentrysdk" to "addons/sentry" ([#180](https://github.com/getsentry/sentry-godot/pull/180))
- Improve thread safety ([#186](https://github.com/getsentry/sentry-godot/pull/186))

### Dependencies

- Bump Native SDK from v0.8.4 to v0.9.0 ([#175](https://github.com/getsentry/sentry-godot/pull/175), [#190](https://github.com/getsentry/sentry-godot/pull/190))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#090)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.8.4...0.9.0)
- Bump gdUnit 4 from v4.5.0 to v5.0.0 ([#185](https://github.com/getsentry/sentry-godot/pull/185))
  - [changelog](https://github.com/MikeSchulze/gdUnit4/blob/master/CHANGELOG.md#v500)
  - [diff](https://github.com/MikeSchulze/gdUnit4/compare/v4.5.0...v5.0.0)

## 0.5.0

### Breaking changes

- Remove `logger` argument from `SentrySDK.capture_message(...)`. This shouldn't be disruptive as the logger argument is seldom used and it had a default value. ([#162](https://github.com/getsentry/sentry-godot/pull/162))

### Features

- Capture scene tree hierarchy data by enabling `attach_scene_tree` option ([#143](https://github.com/getsentry/sentry-godot/pull/143))

### Dependencies

- Bump Native SDK from v0.8.3 to v0.8.4 ([#161](https://github.com/getsentry/sentry-godot/pull/161))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#084)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.8.3...0.8.4)

## 0.4.2

### Various fixes & improvements

- Fix release branch ref in CI (#158) by @limbonaut

## 0.4.1

### Other improvements

- Distribute signed macOS binaries ([#156](https://github.com/getsentry/sentry-godot/pull/156))

## 0.4.0

### Breaking changes

- Renamed `debug_verbosity` => `diagnostic_level` to better align with established Sentry features ([#154](https://github.com/getsentry/sentry-godot/pull/154))
- Mark options as basic and advanced to have a cleaner interface, and move error logger tunables into their own sub-page. This is a BREAKING change so make sure to reapply those error logger values if you're changing the defaults. ([#155](https://github.com/getsentry/sentry-godot/pull/155))

### Features

- Introduce `screenshot_level` option and `before_capture_screenshot` hook to provide fine-grained control over when screenshots are taken. ([#153](https://github.com/getsentry/sentry-godot/pull/153))

## 0.3.1

### Fixes

- Optimize screenshot feature to reduce stutters ([#148](https://github.com/getsentry/sentry-godot/pull/148))
- Don't process screenshot when `attach_screenshot` option is disabled ([#145](https://github.com/getsentry/sentry-godot/pull/145))
- Add missing documentation for `debug_verbosity` option ([#147](https://github.com/getsentry/sentry-godot/pull/147))

### Dependencies

- Bump Native SDK from v0.8.1 to v0.8.3 ([#144](https://github.com/getsentry/sentry-godot/pull/144), [#152](https://github.com/getsentry/sentry-godot/pull/152))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#083)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.8.1...0.8.3)

## 0.3.0

### Features

- Show debug messages in Godot Output panel ([#135](https://github.com/getsentry/sentry-godot/pull/135))

### Fixes

- Fixed error logger not working properly on macOS and not showing debug output in the Output panel ([#138](https://github.com/getsentry/sentry-godot/pull/138))
- Fixed demo project failing to initialize in exported environment ([#141](https://github.com/getsentry/sentry-godot/pull/141))
- Fixed demo output not updating in exported project ([#142](https://github.com/getsentry/sentry-godot/pull/142))

## 0.2.0

### Breaking changes

- Renamed the **Sentry/Config** category in Project Settings to **Sentry/Options** ([#119](https://github.com/getsentry/sentry-godot/pull/119)). This change invalidates all previously set options in project settings. To migrate, open your `project.godot` file in a text editor and replace all instances of "sentry/config" with "sentry/options".

### Features

- In-editor class reference documentation ([#104](https://github.com/getsentry/sentry-godot/pull/104))
- Capture screenshots when enabled via `attach_screenshot` option ([#128](https://github.com/getsentry/sentry-godot/pull/128))

### Fixes

- Fix `user.id` not assigned to `installation_id` by default ([#118](https://github.com/getsentry/sentry-godot/pull/118))
- Don't try to fix crashpad_handler Unix permissions on Windows ([#132](https://github.com/getsentry/sentry-godot/pull/132))

### Dependencies

- Bump Native SDK from v0.7.20 to v0.8.1 ([#126](https://github.com/getsentry/sentry-godot/pull/126), [#133](https://github.com/getsentry/sentry-godot/pull/133))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#081)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.7.20...0.8.1)

## 0.1.2

### Breaking changes

- The SDK no longer automatically persists user data on disk. If you want to persist user data, make sure to save it manually.
- `SentryUser.is_user_valid()` was replaces in favor of `SentryUser.is_empty()`.

### Features

- Add auto debug mode ([#73](https://github.com/getsentry/sentry-godot/pull/73))
- New method `SentrySDK.is_enabled()` ([#82](https://github.com/getsentry/sentry-godot/pull/82))
- Explicitly set `user.ip_address` to "{{auto}}" if `options.send_default_pii` is enabled and the user data is not set in a configuration script ([#101](https://github.com/getsentry/sentry-godot/pull/101))

### Fixes

- Fix issues with exporting crashpad_handler dependency and resolving path to crashpad_handler on macOS in exported projects ([#108](https://github.com/getsentry/sentry-godot/pull/108))

### Dependencies

- Bump Native SDK from v0.7.19 to v0.7.20 ([#84](https://github.com/getsentry/sentry-godot/pull/84))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#0720)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.7.19...0.7.20)

## 0.1.1

### Fixes

- Fix crashes on macOS with GodotSteam in the same project ([#92](https://github.com/getsentry/sentry-godot/pull/92))
- Autofix crashpad handler executable bit permissions on macOS and Linux ([#96](https://github.com/getsentry/sentry-godot/pull/96))
- Fix build warnings on macOS, use newer Xcode & synchronize macOS deployment target for better compatibility with older OS versions ([#93](https://github.com/getsentry/sentry-godot/pull/93))

## 0.1.0

### Features

- Improve & expose `SentryOptions` class ([#56](https://github.com/getsentry/sentry-godot/pull/56))
- Create or modify events using `SentryEvent` objects and new SDK methods: `SentrySDK.create_event()`, `SentrySDK.capture_event(event)` ([#51](https://github.com/getsentry/sentry-godot/pull/51))
- New `environment` property in `SentryOptions` and better auto-naming to prioritize development environments ([#66](https://github.com/getsentry/sentry-godot/pull/66))
- Configure the SDK via GDScript and filter events using event hooks `before_send` and `on_crash`. The new `SentryConfiguration` class can be extended in a script and assigned in options to configure the SDK during initialization. However, due to the way scripting is initialized in the Godot Engine, this comes with a trade-off: a slightly later initialization. If a configuration script is assigned, initialization must be delayed until ScriptServer is ready to load and run the user script. ([#60](https://github.com/getsentry/sentry-godot/pull/60))
- New `dist` property in `SentryOptions` ([#74](https://github.com/getsentry/sentry-godot/pull/74))
- Click to copy UUIDs in the demo project ([#78](https://github.com/getsentry/sentry-godot/pull/78))
- Customize `SentryEvent` tags ([#72](https://github.com/getsentry/sentry-godot/pull/72))

### Improvements

- Refine sentry-native build step and improve tracking of build artifacts ([#71](https://github.com/getsentry/sentry-godot/pull/71))

### Dependencies

- Bump Native SDK from v0.7.17 to v0.7.19 ([#61](https://github.com/getsentry/sentry-godot/pull/61), [#63](https://github.com/getsentry/sentry-godot/pull/63))
  - [changelog](https://github.com/getsentry/sentry-native/blob/master/CHANGELOG.md#0719)
  - [diff](https://github.com/getsentry/sentry-native/compare/0.7.17...0.7.19)

## 0.0.3

### Various fixes & improvements

- add release registry (cae6ce4c) by @bruno-garcia

## 0.0.2

### Dependencies

- Bump sentry-native to 0.7.17 ([#53](https://github.com/getsentry/sentry-godot/pull/53))

## 0.0.1

Initial release
