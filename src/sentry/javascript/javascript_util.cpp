#include "javascript_util.h"

#include <godot_cpp/classes/java_script_bridge.hpp>

namespace sentry::javascript {

void js_delete_property(const Ref<JavaScriptObject> &p_object, const String &p_key) {
	ERR_FAIL_COND(p_object.is_null());
	// TODO: cache singleton
	Ref<JavaScriptObject> reflect = JavaScriptBridge::get_singleton()->get_interface("Reflect");
	ERR_FAIL_COND(reflect.is_null());
	reflect->call("deleteProperty", p_object, p_key);
}

void js_push_json_to_array(const Ref<JavaScriptObject> &p_array, const String &p_json) {
	ERR_FAIL_COND(p_array.is_null());
	Ref<JavaScriptObject> bridge = JavaScriptBridge::get_singleton()->get_interface("SentryBridge");
	ERR_FAIL_COND(bridge.is_null());
	bridge->call("pushJsonToArray", p_array, p_json);
}

} // namespace sentry::javascript
