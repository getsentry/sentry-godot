#include "android_span.h"

#include "android_string_names.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace {

constexpr const char *OP_KEY = "sentry.op";

inline String _get_op(const Dictionary &p_attributes) {
	return p_attributes.get(OP_KEY, String());
}

} // unnamed namespace

namespace sentry::android {

SentrySpanImpl *AndroidSpan::start_root(Object *p_android_plugin, const String &p_name, const Dictionary &p_attributes) {
	ERR_FAIL_NULL_V(p_android_plugin, SentrySpanImpl::create_noop());
	int32_t new_handle = p_android_plugin->call(ANDROID_SN(startSpan), p_name, _get_op(p_attributes));
	if (new_handle == 0) {
		return SentrySpanImpl::create_noop();
	}
	return memnew(AndroidSpan(p_android_plugin, new_handle, p_attributes));
}

SentrySpanImpl *AndroidSpan::start_child(const String &p_name, const Dictionary &p_attributes) {
	int32_t child_handle = _android_plugin->call(ANDROID_SN(spanStartChild), _handle, p_name, _get_op(p_attributes));
	if (child_handle == 0) {
		return SentrySpanImpl::create_noop();
	}
	return memnew(AndroidSpan(_android_plugin, child_handle, p_attributes));
}

void AndroidSpan::set_attribute(const String &p_key, const Variant &p_value) {
	// Type-specific call paths, because Godot bindings can't pass an arbitrary object value.
	switch (p_value.get_type()) {
		case Variant::Type::BOOL: {
			_android_plugin->call(ANDROID_SN(spanSetAttributeBool), _handle, p_key, p_value);
		} break;
		case Variant::Type::INT: {
			_android_plugin->call(ANDROID_SN(spanSetAttributeLong), _handle, p_key, p_value);
		} break;
		case Variant::Type::FLOAT: {
			_android_plugin->call(ANDROID_SN(spanSetAttributeDouble), _handle, p_key, p_value);
		} break;
		case Variant::Type::STRING: {
			_android_plugin->call(ANDROID_SN(spanSetAttributeString), _handle, p_key, p_value);
		} break;
		default: {
			_android_plugin->call(ANDROID_SN(spanSetAttributeString), _handle, p_key, p_value.stringify());
		} break;
	}
}

void AndroidSpan::set_status(SpanStatus p_status) {
	_android_plugin->call(ANDROID_SN(spanSetStatus), _handle, p_status);
}

void AndroidSpan::end() {
	_android_plugin->call(ANDROID_SN(spanEnd), _handle);
}

PackedStringArray AndroidSpan::get_trace_headers() {
	return _android_plugin->call(ANDROID_SN(spanGetTraceHeaders), _handle);
}

void AndroidSpan::_apply_attributes(const Dictionary &p_attributes) {
	const Array &keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		const Variant &key = keys[i];
		String name = key;
		ERR_CONTINUE_MSG(name.is_empty(), "Sentry: Can't set attribute with an empty key.");
		set_attribute(name, p_attributes[key]);
	}
}

AndroidSpan::AndroidSpan(Object *p_android_plugin, int32_t p_handle, const Dictionary &p_attributes) :
		_android_plugin(p_android_plugin), _handle(p_handle) {
	_apply_attributes(p_attributes);
}

AndroidSpan::~AndroidSpan() {
	_android_plugin->call(ANDROID_SN(releaseSpan), _handle);
}

} //namespace sentry::android
