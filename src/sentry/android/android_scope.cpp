#include "android_scope.h"

#include "android_breadcrumb.h"
#include "android_string_names.h"
#include "android_util.h"

#include <godot_cpp/core/error_macros.hpp>

namespace sentry::android {

void AndroidScope::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(scopeSetContext), handle, p_key, sanitize_variant(p_value));
}

void AndroidScope::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(scopeSetTag), handle, p_key, p_value);
}

void AndroidScope::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_NULL(android_plugin);

	if (p_user.is_valid()) {
		android_plugin->call(ANDROID_SN(scopeSetUser), handle,
				p_user->get_id(),
				p_user->get_username(),
				p_user->get_email(),
				p_user->get_ip_address());
	} else {
		android_plugin->call(ANDROID_SN(scopeRemoveUser), handle);
	}
}

void AndroidScope::set_level(sentry::Level p_level) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(scopeSetLevel), handle, p_level);
}

void AndroidScope::set_fingerprint(const PackedStringArray &p_fingerprint) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(scopeSetFingerprint), handle, p_fingerprint);
}

void AndroidScope::set_attribute(const String &p_name, const Variant &p_value) {
	ERR_FAIL_NULL(android_plugin);

	// Use a type-specific call path because Godot bindings don't natively support
	// passing arbitrary object/any values.
	switch (p_value.get_type()) {
		case Variant::Type::BOOL: {
			android_plugin->call(ANDROID_SN(scopeSetAttributeBool), handle, p_name, p_value);
		} break;
		case Variant::Type::INT: {
			android_plugin->call(ANDROID_SN(scopeSetAttributeLong), handle, p_name, p_value);
		} break;
		case Variant::Type::FLOAT: {
			android_plugin->call(ANDROID_SN(scopeSetAttributeDouble), handle, p_name, p_value);
		} break;
		case Variant::Type::STRING: {
			android_plugin->call(ANDROID_SN(scopeSetAttributeString), handle, p_name, p_value);
		} break;
		default: {
			android_plugin->call(ANDROID_SN(scopeSetAttributeString), handle, p_name, p_value.stringify());
		} break;
	}
}

void AndroidScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ERR_FAIL_NULL(android_plugin);
	Ref<AndroidBreadcrumb> crumb = p_breadcrumb;
	ERR_FAIL_COND(crumb.is_null());
	android_plugin->call(ANDROID_SN(scopeAddBreadcrumb), handle, crumb->get_handle());
}

void AndroidScope::clear() {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(scopeClear), handle);
}

SentryScopeImpl *AndroidScope::clone() const {
	ERR_FAIL_NULL_V(android_plugin, memnew(AndroidScope()));
	int32_t new_handle = android_plugin->call(ANDROID_SN(cloneScope), handle);
	return memnew(AndroidScope(android_plugin, new_handle));
}

AndroidScope::AndroidScope(Object *p_android_plugin, int32_t p_handle) :
		android_plugin(p_android_plugin), handle(p_handle) {
	ERR_FAIL_NULL(p_android_plugin);
}

AndroidScope::~AndroidScope() {
	if (android_plugin) {
		android_plugin->call(ANDROID_SN(releaseScope), handle);
	}
}

} //namespace sentry::android
