#include "sentry_scope.h"

#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

namespace sentry {

void SentryScope::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Contexts cannot have an empty key.");
	_impl->set_context(p_key, p_value);
}

void SentryScope::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Tags cannot have an empty key.");
	_impl->set_tag(p_key, p_value);
}

void SentryScope::set_user(const Ref<SentryUser> &p_user) {
	_impl->set_user(p_user);
}

void SentryScope::set_level(sentry::Level p_level) {
	ERR_FAIL_COND_MSG(p_level < LEVEL_DEBUG || p_level > LEVEL_FATAL, "Invalid level.");
	_impl->set_level(p_level);
}

void SentryScope::set_fingerprint(PackedStringArray p_fingerprint) {
	ERR_FAIL_COND_MSG(p_fingerprint.is_empty(), "Fingerprint cannot be empty.");
	_impl->set_fingerprint(p_fingerprint);
}

void SentryScope::set_attribute(const String &p_name, const Variant &p_value) {
	ERR_FAIL_COND_MSG(p_name.is_empty(), "Attribute name cannot be empty.");
	_impl->set_attribute(p_name, p_value);
}

void SentryScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ERR_FAIL_COND_MSG(p_breadcrumb.is_null(), "Breadcrumb cannot be null.");
	_impl->add_breadcrumb(p_breadcrumb);
}

void SentryScope::clear() {
	// Event processors MUST persist across clear().
	_impl->clear();
}

Ref<SentryScope> SentryScope::clone() const {
	return Ref<SentryScope>(memnew(SentryScope(_impl->clone())));
}

void SentryScope::add_event_processor(const Callable &p_callable) {
	ERR_FAIL_COND_MSG(p_callable.is_null(), "Event processor cannot be null.");
	_processors.push_back(p_callable);
}

void SentryScope::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_context", "key", "value"), &SentryScope::set_context);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &SentryScope::set_tag);
	ClassDB::bind_method(D_METHOD("set_user", "user"), &SentryScope::set_user);
	ClassDB::bind_method(D_METHOD("set_level", "level"), &SentryScope::set_level);
	ClassDB::bind_method(D_METHOD("set_fingerprint", "fingerprint"), &SentryScope::set_fingerprint);
	ClassDB::bind_method(D_METHOD("set_attribute", "name", "value"), &SentryScope::set_attribute);
	ClassDB::bind_method(D_METHOD("add_breadcrumb", "breadcrumb"), &SentryScope::add_breadcrumb);
	ClassDB::bind_method(D_METHOD("add_event_processor", "callable"), &SentryScope::add_event_processor);
	ClassDB::bind_method(D_METHOD("clear"), &SentryScope::clear);
}

SentryScope::SentryScope() {
	_impl = INTERNAL_SDK()->create_scope();
}

SentryScope::SentryScope(SentryScopeImpl *p_impl) :
		_impl(p_impl) {
}

SentryScope::~SentryScope() {
	memdelete(_impl);
}

} //namespace sentry
