#pragma once

#include <godot_cpp/godot.hpp>

namespace sentry::util {

// Checks if it's safe to query memory info on the current platform.
// Godot 4.5 queries unsupported sysctl keys on iOS (https://github.com/godotengine/godot/pull/109112)
// producing an error.
inline bool safe_to_query_memory_info() {
#ifdef WEB_ENABLED
	// Querying Web memory info emits an unsupported syscall warning.
	return false;
#elif defined(IOS_ENABLED)
	const auto &version = ::godot::gdextension_interface::godot_version;
	if (version.major == 4 && version.minor == 5) {
		return false;
	}
#endif
	return true;
}

} // namespace sentry::util
