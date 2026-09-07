#include "sentry_span.h"

#include "sentry/disabled/disabled_span.h"
#include "sentry/engine_lifecycle/engine_lifecycle.h"
#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#define WRONG_THREAD_MSG \
	"Sentry: Span methods must be called on the thread that created the span."

#define ENDED_SPAN_MSG \
	"Sentry: Can't modify a span that has ended."

namespace {

bool _is_propagation_target(const String &p_url) {
	const String url = p_url.to_lower();
	const Array targets = SENTRY_OPTIONS()->get_trace_propagation_targets();
	for (const Variant &target : targets) {
		if (target.get_type() == Variant::STRING) {
			const String text = target;
			if (text == ".*" || url.contains(text.to_lower())) {
				return true;
			}
		} else {
			ERR_CONTINUE_MSG(target.get_type() != Variant::OBJECT, "Sentry: Ignoring an invalid trace propagation target.");
			Object *object = target;
			RegEx *regex = Object::cast_to<RegEx>(object);
			ERR_CONTINUE_MSG(regex == nullptr || !regex->is_valid(), "Sentry: Ignoring an invalid trace propagation target.");
			if (regex->search(p_url).is_valid()) {
				return true;
			}
		}
	}
	return false;
}

Ref<sentry::SentrySpan> _unassigned_sentinel;

void _release_unassigned_sentinel() {
	_unassigned_sentinel.unref();
}

} // unnamed namespace

namespace sentry {

Ref<SentrySpan> SentrySpan::create_noop() {
	SentrySpanImpl *impl = memnew(DisabledSpan);
	return Ref<SentrySpan>(memnew(SentrySpan(impl)));
}

Ref<SentrySpan> SentrySpan::unassigned() {
	// FYI: Internal SDK is not initialized yet when this is first called, which
	// happens while binding methods, since it is the default value for start_span().
	// That first call is also what makes the unsynchronized init safe: it happens on
	// the main thread during class registration, before any thread can reach start_span().
	if (_unassigned_sentinel.is_null()) {
		_unassigned_sentinel = create_noop();
		// Holding it until process exit would outlive ObjectDB and crash on teardown.
		engine_lifecycle::add_module_termination_callback(callable_mp_static(&_release_unassigned_sentinel));
	}
	return _unassigned_sentinel;
}

void SentrySpan::set_attribute(const String &p_key, const Variant &p_value) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	ERR_FAIL_COND_MSG(_ended, ENDED_SPAN_MSG);
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set attribute with an empty key.");
	_impl->set_attribute(p_key, p_value);
}

void SentrySpan::set_attributes(const Dictionary &p_attributes) {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	ERR_FAIL_COND_MSG(_ended, ENDED_SPAN_MSG);
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
	ERR_FAIL_COND_MSG(_ended, ENDED_SPAN_MSG);
	_impl->set_status(p_status);
}

PackedStringArray SentrySpan::get_trace_headers(const String &p_url) {
	ERR_SENTRY_THREAD_GUARD_V(PackedStringArray(), WRONG_THREAD_MSG);
	ERR_FAIL_COND_V_MSG(_ended, PackedStringArray(), "Sentry: Can't read trace headers from a span that has ended.");
	if (!p_url.is_empty() && !_is_propagation_target(p_url)) {
		return PackedStringArray();
	}
	return _impl->get_trace_headers();
}

void SentrySpan::end() {
	ERR_SENTRY_THREAD_GUARD(WRONG_THREAD_MSG);
	if (_ended) {
		return;
	}
	_ended = true;
	if (SentrySDK *sdk = SentrySDK::get_singleton()) {
		sdk->notify_span_ended(this);
	}
	_impl->end();
}

void SentrySpan::set_associated_scope(const Ref<SentryScope> &p_scope) {
	_scope_id = p_scope.is_valid() ? p_scope->get_instance_id() : 0;
}

Ref<SentryScope> SentrySpan::get_associated_scope() const {
	return Ref<SentryScope>(Object::cast_to<SentryScope>(ObjectDB::get_instance(_scope_id)));
}

Ref<SentrySpan> SentrySpan::start_child(const String &p_name, const Dictionary &p_attributes) {
	ERR_SENTRY_THREAD_GUARD_V(create_noop(), WRONG_THREAD_MSG);
	ERR_FAIL_COND_V_MSG(_ended, create_noop(), "Sentry: Can't start a child span on a span that has ended.");
	ERR_FAIL_COND_V_MSG(p_name.is_empty(), create_noop(), "Sentry: Can't start a child span with an empty name.");
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
	ClassDB::bind_method(D_METHOD("get_trace_headers", "url"), &SentrySpan::get_trace_headers, DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("end"), &SentrySpan::end);

	BIND_ENUM_CONSTANT(SPAN_STATUS_OK);
	BIND_ENUM_CONSTANT(SPAN_STATUS_ERROR);
}

} // namespace sentry

#undef WRONG_THREAD_MSG
#undef ENDED_SPAN_MSG
