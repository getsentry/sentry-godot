#include "native_scope.h"

#include "sentry/native/native_breadcrumb.h"
#include "sentry/native/native_span.h"
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
	sentry_scope_set_user(_scope, user_to_sentry_value(p_user));
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
	ERR_FAIL_NULL(native_crumb);
	sentry_value_t crumb_value = native_crumb->get_native_breadcrumb();
	sentry_value_incref(crumb_value); // Scope takes ownership.
	sentry_scope_add_breadcrumb(_scope, crumb_value);
}

void NativeScope::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	sentry_attachment_t *native_attachment = nullptr;

	if (!p_attachment->get_path().is_empty()) {
		String absolute_path = p_attachment->get_globalized_path();

		native_attachment = sentry_scope_attach_file(_scope, absolute_path.utf8());
		ERR_FAIL_NULL_MSG(native_attachment, vformat("Sentry: Failed to attach file: %s", absolute_path));

		if (!p_attachment->get_filename().is_empty()) {
			sentry_attachment_set_filename(native_attachment, p_attachment->get_filename().utf8());
		}
	} else {
		PackedByteArray bytes = p_attachment->get_bytes();

		native_attachment = sentry_scope_attach_bytes(_scope,
				reinterpret_cast<const char *>(bytes.ptr()),
				bytes.size(),
				p_attachment->get_filename().utf8());
		ERR_FAIL_NULL_MSG(native_attachment, vformat("Sentry: Failed to attach bytes with filename: %s", p_attachment->get_filename()));
	}

	if (!p_attachment->get_content_type().is_empty()) {
		sentry_attachment_set_content_type(native_attachment, p_attachment->get_content_type().utf8());
	}
}

void NativeScope::clear() {
	sentry_scope_clear(_scope);
}

SentryScopeImpl *NativeScope::clone() const {
	return memnew(NativeScope(sentry_scope_clone(_scope)));
}

void NativeScope::set_span(SentrySpanImpl *p_span) {
	if (NativeSpan *native_span = Castable::cast_to<NativeSpan>(p_span)) {
		native_span->bind_to_scope(_scope);
	} else {
		sentry_scope_set_span(_scope, nullptr);
	}
}

NativeScope::NativeScope() {
	_scope = sentry_scope_new();
}

NativeScope::NativeScope(sentry_scope_t *p_scope) :
		_scope(p_scope) {
}

NativeScope::~NativeScope() {
	sentry_scope_free(_scope);
}

} // namespace sentry::native
