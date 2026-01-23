#include "javascript_util.h"

#include "sentry/util/json_writer.h"

#include <godot_cpp/classes/java_script_bridge.hpp>

namespace sentry::javascript {

String attributes_to_json(const Dictionary &p_attributes) {
	if (p_attributes.is_empty()) {
		return String();
	}

	util::JSONWriter writer;
	writer.begin_object();

	Array keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		Variant value = p_attributes[key];

		writer.key(key);
		switch (value.get_type()) {
			case Variant::BOOL: {
				writer.value_bool((bool)value);
			} break;
			case Variant::INT: {
				writer.value_int((int64_t)value);
			} break;
			case Variant::FLOAT: {
				writer.value_float((double)value);
			} break;
			case Variant::STRING:
			case Variant::STRING_NAME: {
				writer.value_string((String)value);
			} break;
			default: {
				// Stringify other types
				writer.value_string(value.stringify());
			} break;
		}
	}

	writer.end_object();
	return writer.get_string();
}

Ref<JavaScriptObject> js_sentry_bridge() {
	static Ref<JavaScriptObject> bridge;
	if (unlikely(bridge.is_null())) {
		bridge = JavaScriptBridge::get_singleton()->get_interface("SentryBridge");
	}
	ERR_FAIL_COND_V_MSG(bridge.is_null(), Ref<JavaScriptObject>(), "SentryBridge JS interface not found!");
	return bridge;
}

void js_delete_property(const Ref<JavaScriptObject> &p_object, const String &p_key) {
	ERR_FAIL_COND(p_object.is_null());
	// TODO: cache singleton
	Ref<JavaScriptObject> reflect = JavaScriptBridge::get_singleton()->get_interface("Reflect");
	ERR_FAIL_COND(reflect.is_null());
	reflect->call("deleteProperty", p_object, p_key);
}

void js_push_json_to_array(const Ref<JavaScriptObject> &p_array, const String &p_json) {
	ERR_FAIL_COND(p_array.is_null());
	Ref<JavaScriptObject> bridge = js_sentry_bridge();
	ERR_FAIL_COND(bridge.is_null());
	bridge->call("pushJsonToArray", p_array, p_json);
}

String js_object_to_json(const Ref<JavaScriptObject> &p_object) {
	ERR_FAIL_COND_V(p_object.is_null(), String());
	Ref<JavaScriptObject> bridge = js_sentry_bridge();
	ERR_FAIL_COND_V(bridge.is_null(), String());
	return bridge->call("objectToJson", p_object);
}

void js_merge_json_into_object(const Ref<JavaScriptObject> &p_target, const String &p_json) {
	ERR_FAIL_COND(p_target.is_null());
	Ref<JavaScriptObject> bridge = js_sentry_bridge();
	ERR_FAIL_COND(bridge.is_null());
	bridge->call("mergeJsonIntoObject", p_target, p_json);
}

} // namespace sentry::javascript
