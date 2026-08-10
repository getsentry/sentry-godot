#include "native_span.h"

#include "sentry/disabled/disabled_span.h"

#include <godot_cpp/core/error_macros.hpp>

namespace sentry::native {

void NativeSpan::set_attribute(const String &p_key, const Variant &p_value) {
	WARN_PRINT_ONCE("Sentry: Not implemented");
}

Variant NativeSpan::get_attribute(const String &p_key) const {
	WARN_PRINT_ONCE("Sentry: Not implemented");
	return Variant();
}

Dictionary NativeSpan::get_attributes() const {
	WARN_PRINT_ONCE("Sentry: Not implemented");
	return Dictionary();
}

void NativeSpan::set_status(SpanStatus p_status) {
	WARN_PRINT_ONCE("Sentry: Not implemented");
}

SpanStatus NativeSpan::get_status() const {
	WARN_PRINT_ONCE("Sentry: Not implemented");
	return SPAN_STATUS_UNSET;
}

void NativeSpan::set_name(const String &p_name) {
	WARN_PRINT_ONCE("Sentry: Not implemented");
}

String NativeSpan::get_name() const {
	WARN_PRINT_ONCE("Sentry: Not implemented");
	return String();
}

void NativeSpan::end() {
	WARN_PRINT_ONCE("Sentry: Not implemented");
}

SentrySpanImpl *NativeSpan::start_child(const String &p_name) {
	WARN_PRINT_ONCE("Sentry: Not implemented");
	return memnew(DisabledSpan);
}

NativeSpan::NativeSpan() {
}

NativeSpan::~NativeSpan() {
}

} //namespace sentry::native
