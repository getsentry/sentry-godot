#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::javascript {

// Convert a Dictionary of attributes to a JSON string.
// Supported types (bool, int, float, string) are preserved, others are stringified.
String attributes_to_json(const Dictionary &p_attributes);

} //namespace sentry::javascript
