#pragma once

#include "sentry/engine_lifecycle/engine_lifecycle.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace sentry::util {

// Storage wrapper for user-supplied Callables (before_send and friends).
// Godot can crash when a GDScript lambda Callable or its backing script is
// destroyed/accessed after the GDScript runtime has shut down.
// See: https://github.com/godotengine/godot/issues/102569
// To avoid this, release the Callable during engine shutdown while GDScript is
// still alive. If that is already too late and the Callable is an invalid custom
// Callable, leak it instead of destroying it to avoid a double-free at exit.
class SafeCallable {
private:
	Callable *callable = nullptr;

	void _release() {
		if (callable == nullptr) {
			return;
		}

		if (callable->is_custom() && !callable->is_valid()) {
			// Possibly orphaned GDScript lambda - its destructor would double-free, so leak it.
			// This shouldn't happen normally, because we release callable during engine shutdown.
			callable = nullptr;
		} else {
			memdelete(callable);
			callable = nullptr;
		}
	}

public:
	void set(const Callable &p_callable) {
		_release();
		callable = memnew(Callable(p_callable));
	}

	Callable get() const {
		return callable ? *callable : Callable();
	}

	SafeCallable() {
		// Release during engine shutdown while GDScript is still alive to avoid exit-time crashes.
		sentry::engine_lifecycle::add_shutdown_callback(Callback<>::bind<&SafeCallable::_release>(this));
	}
	SafeCallable(const SafeCallable &) = delete;
	SafeCallable &operator=(const SafeCallable &) = delete;
	~SafeCallable() {
		sentry::engine_lifecycle::remove_shutdown_callback(Callback<>::bind<&SafeCallable::_release>(this));
		_release();
	}
};

} //namespace sentry::util
