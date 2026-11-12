# sentry-godot Review Context

## Build-Time Backend Selection

`SConstruct` picks one backend per build and defines `SDK_NATIVE`, `SDK_COCOA`,
`SDK_ANDROID`, or `SDK_JAVASCRIPT`. Only that backend's sources compile, so a
change in one backend never breaks another at compile time. Drift shows up only
at runtime. `linux`/`windows` on `arm32` or `rv64` get the disabled backend.
On Android, the editor build and a missing plugin also fall back to `DisabledSDK`.

Shared interfaces each backend implements:

- `InternalSDK` (`src/sentry/internal_sdk.h`): SDK-level calls. `set_tags` has a
  default loop over `set_tag`; overrides must keep merge semantics.
- `SentryScopeImpl`, `SentrySpanImpl`: owned by the Godot-facing `SentryScope` and
  `SentrySpan`. Backends without span support leave span binding unimplemented.
- Per-backend wrappers for event, breadcrumb, log, and metric objects.

When reviewing a backend change, grep the same method name in the other backend
directories and in `SentryAndroidGodotPlugin.kt` and `sentry-bridge.ts`.

## Android Bridge

C++ calls the Kotlin plugin through `plugin->call(ANDROID_SN(name), ...)`.
Kotlin keeps Java objects in handle maps keyed by `int32_t`. Every `create*`
handle needs a matching `release*` on all paths, including early returns after
user callbacks. Method names are strings in `android_string_names.h`; a rename on
one side fails only at runtime.

## Web Bridge

`javascript_*.cpp` calls into `sentry-bridge.ts` through Godot's JavaScript
bridge. The bridge owns `@sentry/browser` state: global, isolation, and current
scopes. Re-init must reset all three where the change writes state.

## .NET Layer

`csharp_interop.cpp` exposes `extern "C"` functions with explicit lengths
(`int32_t`). Managed code is in `src/sentry/dotnet/managed/Sentry.Godot/`
(`Interop/NativeBridge.cs`, `SentrySdk.cs`). Native and managed init and close can
start from either side. Check re-entry flags (`_initializing`, `_closing`) and
that native close runs once. Managed callbacks are not callable after the script
runtime is torn down.

## Scopes And Threads

- Scopes are thread-local (`SentrySDK::current_scopes`). `init()` and `close()`
  bump `scopes_epoch` so other threads drop stale stacks lazily on next access.
  Scope refs can outlive `close()`; that is expected.
- `ThreadOwner` and `ERR_SENTRY_THREAD_GUARD` reject calls from a thread other
  than the creator's. New public methods on guarded classes need the guard.
- Godot logger callbacks can run on any thread (see the `thread_local` guards in
  `sentry_godot_logger.cpp`), and Cocoa can capture transactions on a background
  queue. Code reached from them must not touch the scene tree directly; it uses
  `call_deferred` or the existing thread-safe paths.
- `logging::in_message_logging` prevents recursion when the SDK logs while
  handling a log.

## Lifecycle

- `engine_lifecycle` registers shutdown callbacks that run before the script
  runtime is torn down. A `NOTIFICATION_PREDELETE` fallback covers cases where the
  scene-tree watcher never ran. Android JNI singletons die before scene-tree
  cleanup, so Android shutdown must run in the early callback.
- Do not touch engine singletons before `are_engine_singletons_ready()`.
- Callables bound with `LIFETIME_OBSERVABLE` / `CallableCpp` stop running after the
  target is destroyed or calls `_invalidate_lifetime()`.
- `ObjectHandle<T>` is non-owning; `get()` can return null.
- Live extension reload is not supported.

## Public API And Settings

- Bound methods are in each class's `_bind_methods`. Class docs live in
  `doc_classes/*.xml`, and CI fails when they are stale, so do not report missing
  docs.
- Project settings live under `sentry/options/...`. Renames are handled by
  versioned steps in `settings_migrations.cpp` (`schema_version`). A moved key
  without a migration or fallback read silently resets user configuration.
- Minimum Godot is `COMPATIBILITY_MINIMUM` in `SConstruct` (4.5), passed to
  godot-cpp as `api_version`, so newer engine APIs fail to compile. CI runs tests
  on 4.5, 4.6, and 4.7; runtime differences between them still matter.
- Deprecated options are kept as no-ops for compatibility; do not report them as
  dead code.
