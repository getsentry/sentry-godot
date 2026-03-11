#pragma once

#include "sentry/javascript/javascript_interop.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::javascript {

// Convert a Dictionary of attributes to a JSON string.
// Supported types (bool, int, float, string) are preserved, others are stringified.
String attributes_to_json(const Dictionary &p_attributes);

Variant sentry_js_object_get_attribute(const JSObjectPtr &p_object, const String &p_name);
void sentry_js_object_set_attribute(const JSObjectPtr &p_object, const String &p_name, const Variant &p_value);

} //namespace sentry::javascript
