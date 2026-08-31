#include "javascript_span.h"

#include "sentry/javascript/javascript_util.h"

#include <godot_cpp/core/error_macros.hpp>

namespace sentry::javascript {

// NOTE: Bridge and js_obj are guaranteed to be valid: creation/bridge failures yield a DisabledSpan instead.

SentrySpanImpl *JavaScriptSpan::start_span(const String &p_name, const Dictionary &p_attributes, const JSObjectPtr &p_parent) {
	String attr_value = attributes_to_json(p_attributes);
	JSObjectPtr span_obj = js_bridge()->call("startSpan", p_name.utf8(), attr_value.utf8(), p_parent).as_object();
	ERR_FAIL_COND_V_MSG(!span_obj, SentrySpanImpl::create_noop(), "Sentry: Failed to create span object.");
	return memnew(JavaScriptSpan(span_obj));
}

SentrySpanImpl *JavaScriptSpan::start_child(const String &p_name, const Dictionary &p_attributes) {
	return start_span(p_name, p_attributes, js_obj);
}

void JavaScriptSpan::set_attribute(const String &p_key, const Variant &p_value) {
	switch (p_value.get_type()) {
		case Variant::Type::BOOL: {
			js_obj->call("setAttribute", p_key.utf8(), p_value.operator bool());
		} break;
		case Variant::Type::INT: {
			js_obj->call("setAttribute", p_key.utf8(), p_value.operator int64_t());
		} break;
		case Variant::Type::FLOAT: {
			js_obj->call("setAttribute", p_key.utf8(), p_value.operator double());
		} break;
		case Variant::Type::STRING: {
			js_obj->call("setAttribute", p_key.utf8(), p_value.operator String().utf8());
		} break;
		default: {
			js_obj->call("setAttribute", p_key.utf8(), p_value.stringify().utf8());
		} break;
	}
}

void JavaScriptSpan::set_status(SpanStatus p_status) {
	js_bridge()->call("spanSetStatus", js_obj, int64_t(p_status));
}

void JavaScriptSpan::end() {
	js_obj->call("end");
}

JavaScriptSpan::JavaScriptSpan(const JSObjectPtr &p_js_span_object) :
		js_obj(p_js_span_object) {
}

} //namespace sentry::javascript
