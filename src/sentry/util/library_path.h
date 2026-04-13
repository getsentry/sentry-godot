#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/string.hpp>

namespace sentry::util {

inline godot::String get_gdextension_library_path() {
	godot::String library_path;
	godot::internal::gdextension_interface_get_library_path(godot::internal::library, library_path._native_ptr());
	return library_path;
}

} // namespace sentry::util
