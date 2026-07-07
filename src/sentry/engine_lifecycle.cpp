#include "godot_singletons.h"

#include <atomic>

namespace {

// Tracks whether engine singletons are fully initialized and safe to access.
std::atomic<bool> singletons_ready{ false };

} // unnamed namespace

namespace sentry::engine_lifecycle {

void mark_engine_singletons_as_ready() {
	singletons_ready.store(true, std::memory_order_release);
}

bool are_engine_singletons_ready() {
	return singletons_ready.load(std::memory_order_acquire);
}

} // namespace sentry::engine_lifecycle
