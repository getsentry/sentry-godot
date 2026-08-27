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

SentrySpanImpl *NativeSpan::start_root(const String &p_name, const Dictionary &p_attributes) {
	sentry_transaction_context_t *context = sentry_transaction_context_new(p_name.utf8(), _get_op(p_attributes));
	sentry_transaction_t *transaction = sentry_transaction_start(context, sentry_value_new_null());
	if (!transaction) {
		return SentrySpanImpl::create_noop();
	}
	return memnew(NativeSpan(transaction, p_attributes));
}

SentrySpanImpl *NativeSpan::start_child(const String &p_name, const Dictionary &p_attributes) {
	sentry_span_t *child = _transaction
			? sentry_transaction_start_child(_transaction, _get_op(p_attributes), p_name.utf8())
			: sentry_span_start_child(_span, _get_op(p_attributes), p_name.utf8());
	if (!child) {
		return SentrySpanImpl::create_noop();
	}
	return memnew(NativeSpan(child, p_attributes));
}

void NativeSpan::set_attribute(const String &p_key, const Variant &p_value) {
	if (_transaction) {
		sentry_transaction_set_data(_transaction, p_key.utf8(), variant_to_attribute_value(p_value));
	} else {
		sentry_span_set_data(_span, p_key.utf8(), variant_to_attribute_value(p_value));
	}
}

void NativeSpan::set_status(SpanStatus p_status) {
	sentry_span_status_t native_status = p_status == SPAN_STATUS_OK ? SENTRY_SPAN_STATUS_OK : SENTRY_SPAN_STATUS_INTERNAL_ERROR;
	if (_transaction) {
		sentry_transaction_set_status(_transaction, native_status);
	} else {
		sentry_span_set_status(_span, native_status);
	}
}

void NativeSpan::end() {
	if (_transaction) {
		sentry_transaction_finish(_transaction);
		_transaction = nullptr;
	} else {
		sentry_span_finish(_span);
		_span = nullptr;
	}
}

void NativeSpan::bind_to_scope(sentry_scope_t *p_scope) {
	if (_transaction) {
		sentry_scope_set_transaction_object(p_scope, _transaction);
	} else {
		sentry_scope_set_span(p_scope, _span);
	}
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

NativeSpan::NativeSpan(sentry_transaction_t *p_transaction, const Dictionary &p_attributes) :
		_transaction(p_transaction) {
	_apply_attributes(p_attributes);
}

NativeSpan::NativeSpan(sentry_span_t *p_span, const Dictionary &p_attributes) :
		_span(p_span) {
	_apply_attributes(p_attributes);
}

NativeSpan::~NativeSpan() {
	if (_transaction) {
		sentry_transaction_discard(_transaction);
	} else if (_span) {
		sentry_span_discard(_span);
	}
}

} //namespace sentry::native
