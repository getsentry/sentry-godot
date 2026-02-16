#include "javascript_util.h"

#include "javascript_string_names.h"
#include "sentry/util/json_writer.h"

#include <godot_cpp/classes/engine.hpp>
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
				writer.value_bool(value.operator bool());
			} break;
			case Variant::INT: {
				writer.value_int(value.operator int64_t());
			} break;
			case Variant::FLOAT: {
				writer.value_float(value.operator double());
			} break;
			case Variant::STRING:
			case Variant::STRING_NAME: {
				writer.value_string(value.operator String());
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
		ERR_FAIL_COND_V_MSG(!Engine::get_singleton()->has_singleton("JavaScriptBridge"), bridge, "JavaScriptBridge singleton is not available but required.");
		bridge = JavaScriptBridge::get_singleton()->get_interface(JAVASCRIPT_SN(SentryBridge));
	}
	return bridge;
}

void js_delete_property(const Ref<JavaScriptObject> &p_object, const String &p_key) {
	ERR_FAIL_COND(p_object.is_null());
	static Ref<JavaScriptObject> reflect;
	if (unlikely(reflect.is_null())) {
		reflect = JavaScriptBridge::get_singleton()->get_interface(JAVASCRIPT_SN(Reflect));
	}
	ERR_FAIL_COND(reflect.is_null());
	reflect->call(JAVASCRIPT_SN(deleteProperty), p_object, p_key);
}

void js_push_json_to_array(const Ref<JavaScriptObject> &p_array, const String &p_json) {
	ERR_FAIL_COND(p_array.is_null());
	Ref<JavaScriptObject> bridge = js_sentry_bridge();
	ERR_FAIL_COND(bridge.is_null());
	bridge->call(JAVASCRIPT_SN(pushJsonObjectToArray), p_array, p_json);
}

String js_object_to_json(const Ref<JavaScriptObject> &p_object) {
	ERR_FAIL_COND_V(p_object.is_null(), String());
	Ref<JavaScriptObject> bridge = js_sentry_bridge();
	ERR_FAIL_COND_V(bridge.is_null(), String());
	return bridge->call(JAVASCRIPT_SN(objectToJson), p_object);
}

void js_merge_json_into_object(const Ref<JavaScriptObject> &p_target, const String &p_json) {
	ERR_FAIL_COND(p_target.is_null());
	Ref<JavaScriptObject> bridge = js_sentry_bridge();
	ERR_FAIL_COND(bridge.is_null());
	bridge->call(JAVASCRIPT_SN(mergeJsonIntoObject), p_target, p_json);
}

double js_object_get_double(const Ref<JavaScriptObject> &p_object, const String &p_key, double p_default) {
	ERR_FAIL_COND_V(p_object.is_null(), p_default);
	Ref<JavaScriptObject> bridge = js_sentry_bridge();
	ERR_FAIL_COND_V(bridge.is_null(), p_default);
	String value_str = bridge->call(JAVASCRIPT_SN(getDoubleAsString), p_object, p_key);
	if (value_str.is_empty()) {
		return p_default;
	}
	return value_str.to_float();
}

void js_object_set_double(const Ref<JavaScriptObject> &p_object, const String &p_key, double p_value) {
	ERR_FAIL_COND(p_object.is_null());
	Ref<JavaScriptObject> bridge = js_sentry_bridge();
	ERR_FAIL_COND(bridge.is_null());
	String value_str = String::num(p_value, 16);
	bridge->call(JAVASCRIPT_SN(setDoubleFromString), p_object, p_key, value_str);
}

Ref<JavaScriptObject> js_object_get_or_create_object_property(const Ref<JavaScriptObject> &p_object, const StringName &p_property) {
	Ref<JavaScriptObject> prop_obj = p_object->get(p_property);
	if (prop_obj.is_null()) {
		prop_obj = JavaScriptBridge::get_singleton()->create_object(JAVASCRIPT_SN(Object));
		p_object->set(p_property, prop_obj);
	}
	return prop_obj;
}

Ref<JavaScriptObject> js_object_get_or_create_array_property(const Ref<JavaScriptObject> &p_object, const StringName &p_property) {
	Ref<JavaScriptObject> prop_obj = p_object->get(p_property);
	if (prop_obj.is_null()) {
		prop_obj = JavaScriptBridge::get_singleton()->create_object(JAVASCRIPT_SN(Array));
		p_object->set(p_property, prop_obj);
	}
	return prop_obj;
}

} // namespace sentry::javascript
