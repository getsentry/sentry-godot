#include "android_scope.h"

#include <godot_cpp/core/error_macros.hpp>

namespace sentry::android {

void AndroidScope::set_context(const String &p_key, const Dictionary &p_value) {
	WARN_PRINT("Sentry: Not implemented.");
}

void AndroidScope::set_tag(const String &p_key, const String &p_value) {
	WARN_PRINT("Sentry: Not implemented.");
}

void AndroidScope::set_user(const Ref<SentryUser> &p_user) {
	WARN_PRINT("Sentry: Not implemented.");
}

void AndroidScope::set_level(sentry::Level p_level) {
	WARN_PRINT("Sentry: Not implemented.");
}

void AndroidScope::set_fingerprint(const PackedStringArray &p_fingerprint) {
	WARN_PRINT("Sentry: Not implemented.");
}

void AndroidScope::set_attribute(const String &p_name, const Variant &p_value) {
	WARN_PRINT("Sentry: Not implemented.");
}

void AndroidScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	WARN_PRINT("Sentry: Not implemented.");
}

void AndroidScope::clear() {
	WARN_PRINT("Sentry: Not implemented.");
}

SentryScopeImpl *AndroidScope::clone() const {
	WARN_PRINT("Sentry: Not implemented.");
	return memnew(AndroidScope);
}

AndroidScope::AndroidScope() {
	WARN_PRINT("Sentry: Not implemented.");
}

AndroidScope::~AndroidScope() {
	WARN_PRINT("Sentry: Not implemented.");
}

} //namespace sentry::android
