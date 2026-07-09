#include "cocoa_scope.h"

#include <godot_cpp/core/error_macros.hpp>

namespace sentry::cocoa {

void CocoaScope::set_context(const String &p_key, const Dictionary &p_value) {
	WARN_PRINT("Sentry: Not implemented.");
}

void CocoaScope::set_tag(const String &p_key, const String &p_value) {
	WARN_PRINT("Sentry: Not implemented.");
}

void CocoaScope::set_user(const Ref<SentryUser> &p_user) {
	WARN_PRINT("Sentry: Not implemented.");
}

void CocoaScope::set_level(sentry::Level p_level) {
	WARN_PRINT("Sentry: Not implemented.");
}

void CocoaScope::set_fingerprint(const PackedStringArray &p_fingerprint) {
	WARN_PRINT("Sentry: Not implemented.");
}

void CocoaScope::set_attribute(const String &p_name, const Variant &p_value) {
	WARN_PRINT("Sentry: Not implemented.");
}

void CocoaScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	WARN_PRINT("Sentry: Not implemented.");
}

void CocoaScope::clear() {
	WARN_PRINT("Sentry: Not implemented.");
}

SentryScopeImpl *CocoaScope::clone() const {
	WARN_PRINT("Sentry: Not implemented.");
	return memnew(CocoaScope);
}

CocoaScope::CocoaScope() {
	WARN_PRINT("Sentry: Not implemented.");
}

CocoaScope::~CocoaScope() {
	WARN_PRINT("Sentry: Not implemented.");
}

} //namespace sentry::cocoa
