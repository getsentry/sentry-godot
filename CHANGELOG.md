# Changelog

## Unreleased

### Breaking changes

- Renamed the **Sentry/Config** category in Project Settings to **Sentry/Options** ([#119](https://github.com/getsentry/sentry-godot/pull/119)). This change invalidates all previously set options in project settings. To migrate, open your `project.godot` file in a text editor and replace all instances of "sentry/config" with "sentry/options".

### Fixes

- Fix `user.id` not assigned to `installation_id` by default ([#118](https://github.com/getsentry/sentry-godot/pull/118))

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
