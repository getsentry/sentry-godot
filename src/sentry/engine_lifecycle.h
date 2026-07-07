#pragma once

namespace sentry::engine_lifecycle {

// Mark Godot engine singletons as safe to access.
void mark_engine_singletons_as_ready();

// Checks whether engine singletons are fully initialized and safe to access.
// Accessing singletons before they're fully initialized can cause crashes, even with null check guards.
// See detailed explanation in https://github.com/getsentry/sentry-godot/pull/475
bool are_engine_singletons_ready();

} // namespace sentry::engine_lifecycle
