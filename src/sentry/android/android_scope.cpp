#include "android_scope.h"

#include "android_breadcrumb.h"
#include "android_span.h"
#include "android_string_names.h"
#include "android_util.h"

#include <godot_cpp/core/error_macros.hpp>

namespace sentry::android {

void AndroidScope::set_context(const String &p_key, const Dictionary &p_value) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(scopeSetContext), handle, p_key, sanitize_variant(p_value));
}

void AndroidScope::set_tag(const String &p_key, const String &p_value) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(scopeSetTag), handle, p_key, p_value);
}

void AndroidScope::set_user(const Ref<SentryUser> &p_user) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}

	if (p_user.is_valid()) {
		plugin->call(ANDROID_SN(scopeSetUser), handle,
				p_user->get_id(),
				p_user->get_username(),
				p_user->get_email(),
				p_user->get_ip_address());
	} else {
		plugin->call(ANDROID_SN(scopeRemoveUser), handle);
	}
}

void AndroidScope::set_level(sentry::Level p_level) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(scopeSetLevel), handle, p_level);
}

void AndroidScope::set_fingerprint(const PackedStringArray &p_fingerprint) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(scopeSetFingerprint), handle, p_fingerprint);
}

void AndroidScope::set_attribute(const String &p_name, const Variant &p_value) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}

	// Use a type-specific call path because Godot bindings don't natively support
	// passing arbitrary object/any values.
	switch (p_value.get_type()) {
		case Variant::Type::BOOL: {
			plugin->call(ANDROID_SN(scopeSetAttributeBool), handle, p_name, p_value);
		} break;
		case Variant::Type::INT: {
			plugin->call(ANDROID_SN(scopeSetAttributeLong), handle, p_name, p_value);
		} break;
		case Variant::Type::FLOAT: {
			plugin->call(ANDROID_SN(scopeSetAttributeDouble), handle, p_name, p_value);
		} break;
		case Variant::Type::STRING: {
			plugin->call(ANDROID_SN(scopeSetAttributeString), handle, p_name, p_value);
		} break;
		default: {
			plugin->call(ANDROID_SN(scopeSetAttributeString), handle, p_name, p_value.stringify());
		} break;
	}
}

void AndroidScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	Ref<AndroidBreadcrumb> crumb = p_breadcrumb;
	ERR_FAIL_COND(crumb.is_null());
	plugin->call(ANDROID_SN(scopeAddBreadcrumb), handle, crumb->get_handle());
}

void AndroidScope::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}

	if (!p_attachment->get_path().is_empty()) {
		String absolute_path = p_attachment->get_globalized_path();
		plugin->call(ANDROID_SN(scopeAddFileAttachment), handle,
				absolute_path,
				p_attachment->get_effective_filename(),
				p_attachment->get_content_type(),
				p_attachment->get_attachment_type());
	} else {
		plugin->call(ANDROID_SN(scopeAddBytesAttachment), handle,
				p_attachment->get_bytes(),
				p_attachment->get_filename(),
				p_attachment->get_content_type(),
				p_attachment->get_attachment_type());
	}
}

void AndroidScope::set_span(SentrySpanImpl *p_span) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	AndroidSpan *android_span = Castable::cast_to<AndroidSpan>(p_span);
	plugin->call(ANDROID_SN(scopeSetSpan), handle, android_span ? android_span->get_handle() : 0);
}

void AndroidScope::clear() {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(scopeClear), handle);
}

SentryScopeImpl *AndroidScope::clone() const {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return memnew(AndroidScope());
	}
	int32_t new_handle = plugin->call(ANDROID_SN(cloneScope), handle);
	return memnew(AndroidScope(_android_plugin, new_handle));
}

AndroidScope::AndroidScope(util::ObjectHandle<Object> p_android_plugin, int32_t p_handle) :
		_android_plugin(p_android_plugin), handle(p_handle) {
}

AndroidScope::~AndroidScope() {
	if (Object *plugin = _android_plugin.get()) {
		plugin->call(ANDROID_SN(releaseScope), handle);
	}
}

} //namespace sentry::android
