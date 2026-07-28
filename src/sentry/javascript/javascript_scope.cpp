#include "javascript_scope.h"

#include "sentry/javascript/javascript_breadcrumb.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace sentry::javascript {

void JavaScriptScope::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set context with an empty key.");
	ERR_FAIL_COND(!js_obj);
	js_bridge()->call("scopeSetContext", js_obj, p_key.utf8(), JSON::stringify(p_value).utf8());
}

void JavaScriptScope::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set tag with an empty key.");
	ERR_FAIL_COND(!js_obj);
	js_obj->call("setTag", p_key.utf8(), p_value.utf8());
}

void JavaScriptScope::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_COND(!js_obj);

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
	ERR_FAIL_COND(!js_obj);
	js_obj->call("setLevel", level_as_cstring(p_level));
}

void JavaScriptScope::set_fingerprint(const PackedStringArray &p_fingerprint) {
	ERR_FAIL_COND(!js_obj);
	js_bridge()->call("scopeSetFingerprint", js_obj, JSON::stringify(p_fingerprint).utf8());
}

void JavaScriptScope::set_attribute(const String &p_name, const Variant &p_value) {
	ERR_FAIL_COND(!js_obj);
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
	ERR_FAIL_COND(!js_obj);
	Ref<JavaScriptBreadcrumb> crumb = p_breadcrumb;
	ERR_FAIL_COND(crumb.is_null());

	js_obj->call("addBreadcrumb", crumb->get_js_object(), SENTRY_OPTIONS()->get_max_breadcrumbs());
}

void JavaScriptScope::clear() {
	ERR_FAIL_COND(!js_obj);
	js_bridge()->call("scopeClear", js_obj);
}

SentryScopeImpl *JavaScriptScope::clone() const {
	ERR_FAIL_COND_V(!js_obj, memnew(JavaScriptScope));
	JSObjectPtr cloned_obj = js_obj->call("clone").as_object();
	ERR_FAIL_COND_V(!cloned_obj, memnew(JavaScriptScope));
	return memnew(JavaScriptScope(cloned_obj));
}

JavaScriptScope::JavaScriptScope(const JSObjectPtr &p_js_scope_object) :
		js_obj(p_js_scope_object) {
}

JavaScriptScope::JavaScriptScope() {
	ERR_FAIL_COND(!js_bridge());
	js_obj = js_bridge()->call("createScope").as_object();
	ERR_FAIL_COND_MSG(!js_obj, "Sentry: Failed to create scope object.");
}

} //namespace sentry::javascript
