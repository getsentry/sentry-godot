#include "android_log.h"

#include "android_string_names.h"

namespace sentry::android {

LogLevel AndroidLog::get_level() const {
	ERR_FAIL_NULL_V(android_plugin, LOG_LEVEL_INFO);
	Variant result = android_plugin->call(ANDROID_SN(logGetLevel), handle);
	ERR_FAIL_COND_V(result.get_type() != Variant::INT, LOG_LEVEL_INFO);
	return (LogLevel)(int)result;
}

void AndroidLog::set_level(LogLevel p_level) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(logSetLevel), handle, p_level);
}

String AndroidLog::get_body() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(logGetBody), handle);
}

void AndroidLog::set_body(const String &p_body) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(logSetBody), handle, p_body);
}

Variant AndroidLog::get_attribute(const String &p_name) const {
	ERR_FAIL_NULL_V(android_plugin, Variant());
	return android_plugin->call(ANDROID_SN(logGetAttribute), handle, p_name);
}

void AndroidLog::set_attribute(const String &p_name, const Variant &p_value) {
	ERR_FAIL_NULL(android_plugin);

	String value = p_value;
	String type;
	switch (p_value.get_type()) {
		case Variant::BOOL: {
			type = "boolean";
		} break;
		case Variant::INT: {
			type = "integer";
		} break;
		case Variant::FLOAT: {
			type = "double";
		} break;
		default: {
			value = p_value.stringify();
			type = "string";
		} break;
	}

	android_plugin->call(ANDROID_SN(logSetAttribute), handle, p_name, type, value);
}

void AndroidLog::add_attributes(const Dictionary &p_attributes) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(logAddAttributes), handle, p_attributes);
}

void AndroidLog::remove_attribute(const String &p_name) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(logRemoveAttribute), handle, p_name);
}

AndroidLog::AndroidLog() {
	// NOTE: Required for ClassDB registration and compilation to succeed.
	ERR_PRINT("This constructor is not intended for runtime use.");
}

AndroidLog::AndroidLog(Object *p_android_plugin, int32_t p_handle) :
		android_plugin(p_android_plugin), handle(p_handle) {
}

AndroidLog::~AndroidLog() {
	if (android_plugin && !is_borrowed) {
		android_plugin->call(ANDROID_SN(releaseLog), handle);
	}
}

} //namespace sentry::android
