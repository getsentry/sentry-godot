#pragma once

#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace sentry::engine_lifecycle {

// Starts watching the engine lifecycle. Safe to call early.
void start_lifecycle_watch();

// Mark Godot engine singletons as safe to access.
void mark_engine_singletons_as_ready();

// Checks whether engine singletons are fully initialized and safe to access.
// Accessing singletons before they're fully initialized can cause crashes, even with null check guards.
// See detailed explanation in https://github.com/getsentry/sentry-godot/pull/475
bool are_engine_singletons_ready();

// Registers a callback to be invoked once when the engine begins shutting down,
// shortly before the script runtime is torn down.
void add_shutdown_callback(const Callable &p_callback);

// Unregisters shutdown callback.
void remove_shutdown_callback(const Callable &p_callback);

// Registers a callback to be invoked once when this extension is deinitialized.
// Useful for releasing statics.
void add_module_termination_callback(const Callable &p_callback);

// Called from register_types.cpp when the module is deinitialized.
// Runs all registered module termination callbacks and then releases all callbacks.
void notify_module_terminating();

} // namespace sentry::engine_lifecycle
