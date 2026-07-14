#pragma once

#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace sentry::engine_lifecycle {

// Starts watching the engine lifecycle. Safe to call early.
void start_lifecycle_watch();

// Checks whether engine singletons are fully initialized and safe to access.
// Accessing singletons before they're fully initialized can cause crashes, even with null check guards.
// See detailed explanation in https://github.com/getsentry/sentry-godot/pull/475
bool are_engine_singletons_ready();

// Registers a callback to be invoked once when the engine begins shutting down,
// shortly before the script runtime is torn down.
void add_shutdown_callback(const Callable &p_callback);

// Unregisters shutdown callback.
void remove_shutdown_callback(const Callable &p_callback);

} // namespace sentry::engine_lifecycle
