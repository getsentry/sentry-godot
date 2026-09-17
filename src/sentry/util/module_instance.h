#pragma once

#include "sentry/engine_lifecycle/engine_lifecycle.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

namespace sentry::util {

// Manages a single shared instance of T, deleting it automatically when the module terminates.
// Must be created on the main thread after Godot initialization and before use by consumers.
// Access is not synchronized; callers must ensure T is used safely across threads.
template <typename T>
class ModuleInstance {
	inline static T *_instance = nullptr;

	static void _destroy() {
		memdelete(_instance);
		_instance = nullptr;
	}

	ModuleInstance() = delete;

public:
	static void create_once() {
		if (_instance) {
			return;
		}
		_instance = memnew(T);
		engine_lifecycle::add_module_termination_callback(callable_mp_static(&ModuleInstance::_destroy));
	}

	_FORCE_INLINE_ static T &get() {
		DEV_ASSERT(_instance);
		return *_instance;
	}
};

} // namespace sentry::util
