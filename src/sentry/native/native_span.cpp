#include "native_span.h"

#include "sentry/native/native_util.h"

#include <godot_cpp/core/error_macros.hpp>

namespace {

constexpr const char *OP_KEY = "sentry.op";

inline CharString _get_op(const Dictionary &p_attributes) {
	return String(p_attributes.get(OP_KEY, String())).utf8();
}

} // unnamed namespace

namespace sentry::native {

void NativeSpan::set_attribute(const String &p_key, const Variant &p_value) {
	if (!_is_live()) {
		return;
	}
	if (_transaction) {
		sentry_transaction_set_data(_transaction, p_key.utf8(), variant_to_sentry_value(p_value));
	} else {
		sentry_span_set_data(_span, p_key.utf8(), variant_to_sentry_value(p_value));
	}
}

void NativeSpan::set_status(SpanStatus p_status) {
	if (!_is_live()) {
		return;
	}
	if (p_status == SPAN_STATUS_UNSET) {
		WARN_PRINT_ONCE("Sentry: Clearing a span status is not supported on this platform.");
		return;
	}
	sentry_span_status_t native_status = p_status == SPAN_STATUS_OK ? SENTRY_SPAN_STATUS_OK : SENTRY_SPAN_STATUS_INTERNAL_ERROR;
	if (_transaction) {
		sentry_transaction_set_status(_transaction, native_status);
	} else {
		sentry_span_set_status(_span, native_status);
	}
}

void NativeSpan::set_name(const String &p_name) {
	if (!_is_live()) {
		return;
	}
	if (_transaction) {
		sentry_transaction_set_name(_transaction, p_name.utf8());
	} else {
		WARN_PRINT_ONCE("Sentry: Renaming a child span is not supported on this platform - the name is fixed at creation.");
	}
}

void NativeSpan::end() {
	if (!_is_live()) {
		return;
	}
	if (_transaction) {
		sentry_transaction_finish(_transaction);
		_transaction = nullptr;
	} else {
		sentry_span_finish(_span);
		_span = nullptr;
	}
}

SentrySpanImpl *NativeSpan::start_child(const String &p_name, const Dictionary &p_attributes) {
	if (!_is_live()) {
		return SentrySpanImpl::noop();
	}
	sentry_span_t *child = _transaction
			? sentry_transaction_start_child(_transaction, _get_op(p_attributes), p_name.utf8())
			: sentry_span_start_child(_span, _get_op(p_attributes), p_name.utf8());
	return memnew(NativeSpan(child, p_attributes));
}

void NativeSpan::_apply_attributes(const Dictionary &p_attributes) {
	const Array &keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		const Variant &key = keys[i];
		String name = key;
		ERR_CONTINUE_MSG(name.is_empty(), "Sentry: Can't set attribute with an empty key.");
		set_attribute(name, p_attributes[key]);
	}
}

NativeSpan::NativeSpan(const String &p_name, const Dictionary &p_attributes) {
	sentry_transaction_context_t *context = sentry_transaction_context_new(p_name.utf8(), _get_op(p_attributes));
	_transaction = sentry_transaction_start(context, sentry_value_new_null());
	_apply_attributes(p_attributes);
}

NativeSpan::NativeSpan(sentry_span_t *p_span, const Dictionary &p_attributes) :
		_span(p_span) {
	_apply_attributes(p_attributes);
}

NativeSpan::~NativeSpan() {
	if (!_is_live()) {
		return;
	}
	if (_transaction) {
		sentry_transaction_discard(_transaction);
	} else {
		sentry_span_discard(_span);
	}
}

} //namespace sentry::native
