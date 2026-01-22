#include "javascript_util.h"

#include <godot_cpp/classes/java_script_bridge.hpp>

namespace sentry::javascript {

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
