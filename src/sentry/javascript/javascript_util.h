#pragma once

#include <godot_cpp/classes/java_script_object.hpp>

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

namespace sentry::javascript {

// Convert a Dictionary of attributes to a JSON string.
// Supported types (bool, int, float, string) are preserved, others are stringified.
String attributes_to_json(const Dictionary &p_attributes);

// Returns JavaScriptObject representing window.SentryBridge in JavaScript environment.
Ref<JavaScriptObject> js_sentry_bridge();

// Delete a property from a JavaScript object using Reflect.deleteProperty.
void js_delete_property(const Ref<JavaScriptObject> &p_object, const String &p_key);

// Reconstruct a JSON string as an object and place it in a JavaScript array.
void js_push_json_to_array(const Ref<JavaScriptObject> &p_array, const String &p_json);

// Convert a JavaScript object to a JSON string.
String js_object_to_json(const Ref<JavaScriptObject> &p_object);

// Reconstruct a JSON string as an object and merge its properties into a JavaScript object.
void js_merge_json_into_object(const Ref<JavaScriptObject> &p_target, const String &p_json);

} //namespace sentry::javascript
