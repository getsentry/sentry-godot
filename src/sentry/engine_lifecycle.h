#pragma once

namespace sentry::godot_singletons {

// Mark Godot engine singletons as safe to access.
void mark_as_ready();

// Checks whether engine singletons are fully initialized and safe to access.
// Accessing singletons before they're fully initialized can cause crashes, even with null check guards.
// See detailed explanation in https://github.com/getsentry/sentry-godot/pull/475
bool are_ready();

} // namespace sentry::godot_singletons
