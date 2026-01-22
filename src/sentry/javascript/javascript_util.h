#pragma once

#include <godot_cpp/classes/java_script_object.hpp>

using namespace godot;

namespace sentry::javascript {

// Delete a property from a JavaScript object using Reflect.deleteProperty.
void js_delete_property(const Ref<JavaScriptObject> &p_object, const String &p_key);

} //namespace sentry::javascript
