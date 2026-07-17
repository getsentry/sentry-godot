#include "native_scope.h"

#include "sentry/native/native_breadcrumb.h"
#include "sentry/native/native_util.h"

namespace sentry::native {

// NOTE: Input validations are performed by Godot-facing SentryScope.

void NativeScope::set_context(const String &p_key, const Dictionary &p_value) {
	sentry_scope_set_context(_scope, p_key.utf8(), variant_to_sentry_value(p_value));
}

void NativeScope::set_tag(const String &p_key, const String &p_value) {
	sentry_scope_set_tag(_scope, p_key.utf8(), p_value.utf8());
}

void NativeScope::set_user(const Ref<SentryUser> &p_user) {
	if (p_user.is_valid()) {
		sentry_scope_set_user(_scope, user_to_sentry_value(p_user));
	} else {
		sentry_scope_set_user(_scope, sentry_value_new_null());
	}
}

void NativeScope::set_level(sentry::Level p_level) {
	sentry_scope_set_level(_scope, level_to_native(p_level));
}

void NativeScope::set_fingerprint(const PackedStringArray &p_fingerprint) {
	sentry_scope_set_fingerprints(_scope, strings_to_sentry_list(p_fingerprint));
}

void NativeScope::set_attribute(const String &p_name, const Variant &p_value) {
	sentry_scope_set_attribute(_scope, p_name.utf8(), variant_to_attribute(p_value));
}

void NativeScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	NativeBreadcrumb *native_crumb = Object::cast_to<NativeBreadcrumb>(p_breadcrumb.ptr());
	if (native_crumb) {
		sentry_value_t crumb_value = native_crumb->get_native_breadcrumb();
		sentry_value_incref(crumb_value); // Scope takes ownership.
		sentry_scope_add_breadcrumb(_scope, crumb_value);
	}
}

void NativeScope::clear() {
	sentry_scope_clear(_scope);
}

SentryScopeImpl *NativeScope::clone() const {
	return memnew(NativeScope(sentry_scope_clone(_scope)));
}

NativeScope::NativeScope() {
	_scope = sentry_scope_new();
}

NativeScope::NativeScope(sentry_scope_t *_scope) :
		_scope(_scope) {
}

NativeScope::~NativeScope() {
	sentry_scope_free(_scope);
}

} // namespace sentry::native
