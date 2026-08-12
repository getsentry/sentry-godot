#include "sentry_span.h"

#include "sentry/disabled/disabled_span.h"
#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

#define WRONG_THREAD_MSG \
	"Sentry: Span methods must be called on the thread that created the span."

namespace sentry {

Ref<SentrySpan> SentrySpan::create_noop() {
	return Ref<SentrySpan>(memnew(SentrySpan(memnew(DisabledSpan))));
}

Ref<SentrySpan> SentrySpan::unassigned() {
	// FYI: Internal SDK is not initialized yet when this static is created.
	static Ref<SentrySpan> sentinel = create_noop();
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

void SentrySpan::set_status(SpanStatus p_status) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	_impl->set_status(p_status);
}

void SentrySpan::end() {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	if (_ended) {
		return;
	}
	_ended = true;
	_impl->end();
}

Ref<SentrySpan> SentrySpan::start_child(const String &p_name, const Dictionary &p_attributes) {
	ERR_FAIL_COND_V_MSG(p_name.is_empty(), Ref<SentrySpan>(), "Sentry: Can't start a child span with an empty name.");
	SentrySpanImpl *child_impl = _impl->start_child(p_name, p_attributes);
	return memnew(SentrySpan(child_impl));
}

SentrySpan::SentrySpan() {
	// Inert by default: the only paths here are unassigned() and a accidental
	// instantiation by the engine or user, neither of which should start a live span.
	_impl = memnew(DisabledSpan);
}

SentrySpan::SentrySpan(const String &p_name, const Dictionary &p_attributes) {
	_impl = INTERNAL_SDK()->create_span(p_name, p_attributes);
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
	ClassDB::bind_method(D_METHOD("set_status", "status"), &SentrySpan::set_status);
	ClassDB::bind_method(D_METHOD("end"), &SentrySpan::end);

	BIND_ENUM_CONSTANT(SPAN_STATUS_UNSET);
	BIND_ENUM_CONSTANT(SPAN_STATUS_OK);
	BIND_ENUM_CONSTANT(SPAN_STATUS_ERROR);
}

} // namespace sentry

#undef WRONG_THREAD_MSG
