#include "javascript_scope.h"

#include <godot_cpp/core/error_macros.hpp>

namespace sentry::javascript {

void JavaScriptScope::set_context(const String &p_key, const Dictionary &p_value) {
	WARN_PRINT("Sentry: Not implemented.");
}

void JavaScriptScope::set_tag(const String &p_key, const String &p_value) {
	WARN_PRINT("Sentry: Not implemented.");
}

void JavaScriptScope::set_user(const Ref<SentryUser> &p_user) {
	WARN_PRINT("Sentry: Not implemented.");
}

void JavaScriptScope::set_level(sentry::Level p_level) {
	WARN_PRINT("Sentry: Not implemented.");
}

void JavaScriptScope::set_fingerprint(const PackedStringArray &p_fingerprint) {
	WARN_PRINT("Sentry: Not implemented.");
}

void JavaScriptScope::set_attribute(const String &p_name, const Variant &p_value) {
	WARN_PRINT("Sentry: Not implemented.");
}

void JavaScriptScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	WARN_PRINT("Sentry: Not implemented.");
}

void JavaScriptScope::clear() {
	WARN_PRINT("Sentry: Not implemented.");
}

SentryScopeImpl *JavaScriptScope::clone() const {
	WARN_PRINT("Sentry: Not implemented.");
	return memnew(JavaScriptScope);
}

JavaScriptScope::JavaScriptScope() {
	WARN_PRINT("Sentry: Not implemented.");
}

JavaScriptScope::~JavaScriptScope() {
	WARN_PRINT("Sentry: Not implemented.");
}

} //namespace sentry::javascript
