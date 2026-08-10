#include "sentry_span.h"

#include "sentry/disabled/disabled_span.h"
#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

#define WRONG_THREAD_MSG \
	"Sentry: Span methods must be called on the thread that created the span."

namespace sentry {

Ref<SentrySpan> SentrySpan::unassigned() {
	// FYI: Internal SDK is not initialized yet when this static is created.
	static Ref<SentrySpan> sentinel = Ref<SentrySpan>(memnew(SentrySpan(memnew(DisabledSpan))));
	return sentinel;
}

void SentrySpan::set_attribute(const String &p_key, const Variant &p_value) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set attribute with an empty key.");
	_impl->set_attribute(p_key, p_value);
}

void SentrySpan::set_attributes(const Dictionary &p_attributes) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	const Array &keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		const Variant &key = keys[i];
		String name = key;
		ERR_CONTINUE_MSG(name.is_empty(), "Sentry: Can't set attribute with an empty key.");
		_impl->set_attribute(name, p_attributes[key]);
	}
}

Variant SentrySpan::get_attribute(const String &p_key) const {
	ERR_SENTRY_THREAD_GUARD_V(Variant(), WRONG_THREAD_MSG);
	ERR_FAIL_COND_V_MSG(p_key.is_empty(), Variant(), "Sentry: Can't get attribute with an empty key.");
	return _impl->get_attribute(p_key);
}

Dictionary SentrySpan::get_attributes() const {
	ERR_SENTRY_THREAD_GUARD_V(Dictionary(), WRONG_THREAD_MSG);
	return _impl->get_attributes();
}

void SentrySpan::set_status(SpanStatus p_status) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	_impl->set_status(p_status);
}

SentrySpan::SpanStatus SentrySpan::get_status() const {
	ERR_SENTRY_THREAD_GUARD_V(SPAN_STATUS_UNSET, WRONG_THREAD_MSG);
	return _impl->get_status();
}

void SentrySpan::set_name(const String &p_name) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	ERR_FAIL_COND_MSG(p_name.is_empty(), "Sentry: Can't set an empty span name.");
	_impl->set_name(p_name);
}

String SentrySpan::get_name() const {
	ERR_SENTRY_THREAD_GUARD_V(String(), WRONG_THREAD_MSG);
	return _impl->get_name();
}

void SentrySpan::end() {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	_impl->end();
}

Ref<SentrySpan> SentrySpan::start_child(const String &p_name) {
	ERR_FAIL_COND_V_MSG(p_name.is_empty(), Ref<SentrySpan>(), "Sentry: Can't start a child span with an empty name.");
	return memnew(SentrySpan(_impl->start_child(p_name)));
}

SentrySpan::SentrySpan() {
	_impl = INTERNAL_SDK()->create_span();
}

SentrySpan::SentrySpan(SentrySpanImpl *p_impl) :
		_impl(p_impl) {
}

SentrySpan::~SentrySpan() {
	memdelete(_impl);
}

void SentrySpan::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_attribute", "key", "value"), &SentrySpan::set_attribute);
	ClassDB::bind_method(D_METHOD("set_attributes", "attributes"), &SentrySpan::set_attributes);
	ClassDB::bind_method(D_METHOD("get_attribute", "key"), &SentrySpan::get_attribute);
	ClassDB::bind_method(D_METHOD("get_attributes"), &SentrySpan::get_attributes);
	ClassDB::bind_method(D_METHOD("set_status", "status"), &SentrySpan::set_status);
	ClassDB::bind_method(D_METHOD("get_status"), &SentrySpan::get_status);
	ClassDB::bind_method(D_METHOD("set_name", "name"), &SentrySpan::set_name);
	ClassDB::bind_method(D_METHOD("get_name"), &SentrySpan::get_name);
	ClassDB::bind_method(D_METHOD("end"), &SentrySpan::end);

	BIND_ENUM_CONSTANT(SPAN_STATUS_UNSET);
	BIND_ENUM_CONSTANT(SPAN_STATUS_OK);
	BIND_ENUM_CONSTANT(SPAN_STATUS_ERROR);
}

} // namespace sentry

#undef WRONG_THREAD_MSG
