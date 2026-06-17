#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/string.hpp>

namespace sentry::util {

// Returns the FS path of this shared library as it was actually loaded by the OS.
godot::String get_loaded_gdextension_library_path();

} // namespace sentry::util
