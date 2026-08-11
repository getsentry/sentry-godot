#include "sentry_scope.h"

#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

#define WRONG_THREAD_MSG                                               \
	"Sentry: This scope is bound to the thread that created it. "      \
	"Call SentrySDK.get_current_scope() on this thread to access its " \
	"current scope, or use process-wide SentrySDK methods such as "    \
	"SentrySDK.set_tag() to enrich telemetry across all threads."

namespace sentry {

void SentryScope::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set context with an empty key.");
	_impl->set_context(p_key, p_value);
}

void SentryScope::set_tag(const String &p_key, const String &p_value) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set tag with an empty key.");
	_impl->set_tag(p_key, p_value);
}

void SentryScope::set_user(const Ref<SentryUser> &p_user) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	_impl->set_user(p_user);
}

void SentryScope::set_level(sentry::Level p_level) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	_impl->set_level(p_level);
}

void SentryScope::set_fingerprint(const PackedStringArray &p_fingerprint) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	_impl->set_fingerprint(p_fingerprint);
}

void SentryScope::set_attribute(const String &p_name, const Variant &p_value) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	ERR_FAIL_COND_MSG(p_name.is_empty(), "Sentry: Can't set attribute with an empty name.");
	_impl->set_attribute(p_name, p_value);
}

void SentryScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	ERR_FAIL_COND_MSG(p_breadcrumb.is_null(), "Sentry: Can't add a null breadcrumb.");
	_impl->add_breadcrumb(p_breadcrumb);
}

void SentryScope::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	ERR_FAIL_COND_MSG(p_attachment.is_null(), "Sentry: Can't add a null attachment.");
	ERR_FAIL_COND_MSG(p_attachment->get_path().is_empty() && p_attachment->get_filename().is_empty(),
			"Sentry: Can't add bytes attachment without filename.");
	_impl->add_attachment(p_attachment);
}

void SentryScope::clear() {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	_impl->clear();
}

void SentryScope::set_span(const Ref<SentrySpan> &p_span) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	ERR_FAIL_COND_MSG(p_span.is_null(), "Sentry: Can't bind a null span to the scope.");
	p_span->set_previous(get_span());
	_span = p_span;
}

Ref<SentrySpan> SentryScope::get_span() const {
	ERR_SENTRY_THREAD_GUARD_V(Ref<SentrySpan>(), WRONG_THREAD_MSG);
	while (_span.is_valid() && _span->is_ended()) {
		_span = _span->get_previous();
	}
	return _span;
}

Ref<SentryScope> SentryScope::clone() const {
	ERR_SENTRY_THREAD_GUARD_V(Ref<SentryScope>(), WRONG_THREAD_MSG);
	Ref<SentryScope> copy = Ref<SentryScope>(memnew(SentryScope(_impl->clone())));
	copy->_span = get_span();
	return copy;
}

void SentryScope::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_context", "key", "value"), &SentryScope::set_context);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &SentryScope::set_tag);
	ClassDB::bind_method(D_METHOD("set_user", "user"), &SentryScope::set_user);
	ClassDB::bind_method(D_METHOD("set_level", "level"), &SentryScope::set_level);
	ClassDB::bind_method(D_METHOD("set_fingerprint", "fingerprint"), &SentryScope::set_fingerprint);
	ClassDB::bind_method(D_METHOD("set_attribute", "name", "value"), &SentryScope::set_attribute);
	ClassDB::bind_method(D_METHOD("add_breadcrumb", "breadcrumb"), &SentryScope::add_breadcrumb);
	ClassDB::bind_method(D_METHOD("add_attachment", "attachment"), &SentryScope::add_attachment);
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

#undef WRONG_THREAD_MSG
