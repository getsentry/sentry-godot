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

} // namespace sentry::javascript
