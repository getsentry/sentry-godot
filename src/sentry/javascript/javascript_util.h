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

// Get a double property from a JavaScript object, preserving precision by passing as string.
double js_object_get_double(const Ref<JavaScriptObject> &p_object, const String &p_key, double p_default = 0.0);

// Set a double property on a JavaScript object, preserving precision by passing as string.
void js_object_set_double(const Ref<JavaScriptObject> &p_object, const String &p_key, double p_value);

// Get a string property from a JavaScript object, or return a default value if not found.
_FORCE_INLINE_ String js_object_get_property_as_string(const Ref<JavaScriptObject> &p_obj, const String &p_property, const String &p_default = "") {
	Variant val = p_obj->get(p_property);
	return val == Variant() ? p_default : val.operator String();
}

// Set a string property on a JavaScript object, or remove it if the provided value is empty.
_FORCE_INLINE_ void js_object_set_or_remove_string_property(const Ref<JavaScriptObject> &p_obj, const String &p_property, const String &p_value) {
	if (p_value.is_empty()) {
		js_delete_property(p_obj, p_property);
	} else {
		p_obj->set(p_property, p_value);
	}
}

// Returns an object property from a JS object, creating/assigning if missing.
Ref<JavaScriptObject> js_object_get_or_create_object_property(const Ref<JavaScriptObject> &p_object, const StringName &p_property);

// Returns an array property from a JS object, creating/assigning if missing.
Ref<JavaScriptObject> js_object_get_or_create_array_property(const Ref<JavaScriptObject> &p_object, const StringName &p_property);

} //namespace sentry::javascript
