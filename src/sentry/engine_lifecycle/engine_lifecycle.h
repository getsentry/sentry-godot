#pragma once

#include "sentry/util/callback.h"

namespace sentry::engine_lifecycle {

// Mark Godot engine singletons as safe to access.
void mark_engine_singletons_as_ready();

// Checks whether engine singletons are fully initialized and safe to access.
// Accessing singletons before they're fully initialized can cause crashes, even with null check guards.
// See detailed explanation in https://github.com/getsentry/sentry-godot/pull/475
bool are_engine_singletons_ready();

// Checks whether the engine is shutting down, making scripts, and the scene tree unsafe to use.
bool is_shutting_down();

// Registers a callback to be invoked once when the engine begins shutting down,
// shortly before the script runtime is torn down.
void add_shutdown_callback(util::Callback<> p_callback);

// Unregisters shutdown callback.
void remove_shutdown_callback(util::Callback<> p_callback);

// RAII marker for a processing code section that may invoke a user Callable
// (before_send and friends) or otherwise touch the GDScript or C# runtime.
// It tracks in-flight processing so shutdown can wait for each section to
// finish before unblocking the main thread and letting Godot release the
// scripting runtime. This ensures no worker thread is mid-call when the runtime
// is torn down.
class ProcessingSection {
public:
	ProcessingSection();
	~ProcessingSection();

	ProcessingSection(const ProcessingSection &) = delete;
	ProcessingSection &operator=(const ProcessingSection &) = delete;
	ProcessingSection(ProcessingSection &&) = delete;
	ProcessingSection &operator=(ProcessingSection &&) = delete;
};

} // namespace sentry::engine_lifecycle
