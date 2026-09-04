#include "javascript_scope.h"

#include "sentry/disabled/disabled_scope.h"
#include "sentry/javascript/javascript_breadcrumb.h"
#include "sentry/javascript/javascript_span.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace sentry::javascript {

// NOTE: Bridge and js_obj are guaranteed to be valid: creation failures yield a DisabledScope instead.

void JavaScriptScope::set_context(const String &p_key, const Dictionary &p_value) {
	js_bridge()->call("scopeSetContext", js_obj, p_key.utf8(), JSON::stringify(p_value).utf8());
}

void JavaScriptScope::set_tag(const String &p_key, const String &p_value) {
	js_obj->call("setTag", p_key.utf8(), p_value.utf8());
}

void JavaScriptScope::set_user(const Ref<SentryUser> &p_user) {
	if (p_user.is_null()) {
		js_obj->call("setUser", nullptr);
		return;
	}

	js_bridge()->call("scopeSetUser",
			js_obj,
			p_user->get_id().utf8(),
			p_user->get_username().utf8(),
			p_user->get_email().utf8(),
			p_user->get_ip_address().utf8());
}

void JavaScriptScope::set_level(sentry::Level p_level) {
	js_obj->call("setLevel", level_as_cstring(p_level));
}

void JavaScriptScope::set_fingerprint(const PackedStringArray &p_fingerprint) {
	js_bridge()->call("scopeSetFingerprint", js_obj, JSON::stringify(p_fingerprint).utf8());
}

void JavaScriptScope::set_attribute(const String &p_name, const Variant &p_value) {
	switch (p_value.get_type()) {
		case Variant::Type::BOOL: {
			js_obj->call("setAttribute", p_name.utf8(), p_value.operator bool());
		} break;
		case Variant::Type::INT: {
			js_obj->call("setAttribute", p_name.utf8(), p_value.operator int64_t());
		} break;
		case Variant::Type::FLOAT: {
			js_obj->call("setAttribute", p_name.utf8(), p_value.operator double());
		} break;
		case Variant::Type::STRING: {
			js_obj->call("setAttribute", p_name.utf8(), p_value.operator String().utf8());
		} break;
		default: {
			js_obj->call("setAttribute", p_name.utf8(), p_value.stringify().utf8());
		} break;
	}
}

void JavaScriptScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	JavaScriptBreadcrumb *crumb = Object::cast_to<JavaScriptBreadcrumb>(p_breadcrumb.ptr());
	ERR_FAIL_NULL(crumb);

	js_obj->call("addBreadcrumb", crumb->get_js_object(), SENTRY_OPTIONS()->get_max_breadcrumbs());
}

void JavaScriptScope::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	if (!p_attachment->get_path().is_empty()) {
		js_bridge()->call("scopeAddFileAttachment",
				js_obj,
				p_attachment->get_path().utf8(),
				p_attachment->get_effective_filename().utf8(),
				p_attachment->get_content_type().utf8(),
				p_attachment->get_attachment_type().utf8());
	} else {
		js_bridge()->call("scopeAddBytesAttachment",
				js_obj,
				p_attachment->get_effective_filename().utf8(),
				p_attachment->get_bytes(),
				p_attachment->get_content_type_or_default().utf8(),
				p_attachment->get_attachment_type().utf8());
	}
}

void JavaScriptScope::set_span(SentrySpanImpl *p_span) {
	if (JavaScriptSpan *js_span = Castable::cast_to<JavaScriptSpan>(p_span)) {
		js_bridge()->call("scopeSetSpan", js_obj, js_span->get_js_object());
	} else {
		js_bridge()->call("scopeSetSpan", js_obj, nullptr);
	}
}

void JavaScriptScope::clear() {
	JSObjectPtr fresh_obj = js_bridge()->call("scopeClear", js_obj).as_object();
	ERR_FAIL_COND_MSG(!fresh_obj, "Sentry: Failed to clear scope object.");
	js_obj = fresh_obj;
}

SentryScopeImpl *JavaScriptScope::clone() const {
	JSObjectPtr cloned_obj = js_bridge()->call("scopeClone", js_obj).as_object();
	ERR_FAIL_COND_V_MSG(!cloned_obj, memnew(DisabledScope), "Sentry: Failed to clone scope object.");
	return memnew(JavaScriptScope(cloned_obj));
}

JavaScriptScope::JavaScriptScope(const JSObjectPtr &p_js_scope_object) :
		js_obj(p_js_scope_object) {
}

} //namespace sentry::javascript
