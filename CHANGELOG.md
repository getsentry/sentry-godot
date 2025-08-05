# Changelog

## Unreleased

### Features

- Intoduce `SentryTimestamp` class to improve timestamp handling ([#286](https://github.com/getsentry/sentry-godot/pull/286
- Support for iOS & macOS using Sentry Cocoa SDK integration ([#266](https://github.com/getsentry/sentry-godot/pull/266))

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
