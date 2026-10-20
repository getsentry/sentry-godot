#include "android_log.h"

#include "android_string_names.h"

namespace sentry::android {

LogLevel AndroidLog::get_level() const {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return LOG_LEVEL_INFO;
	}
	Variant result = plugin->call(ANDROID_SN(logGetLevel), handle);
	ERR_FAIL_COND_V(result.get_type() != Variant::INT, LOG_LEVEL_INFO);
	return (LogLevel)(int)result;
}

void AndroidLog::set_level(LogLevel p_level) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(logSetLevel), handle, p_level);
}

String AndroidLog::get_body() const {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return String();
	}
	return plugin->call(ANDROID_SN(logGetBody), handle);
}

void AndroidLog::set_body(const String &p_body) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(logSetBody), handle, p_body);
}

Variant AndroidLog::get_attribute(const String &p_name) const {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return Variant();
	}
	Dictionary result = plugin->call(ANDROID_SN(logGetAttribute), handle, p_name);
	return result["value"];
}

void AndroidLog::set_attribute(const String &p_name, const Variant &p_value) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}

	Dictionary attr_data;
	attr_data["name"] = p_name;

	switch (p_value.get_type()) {
		case Variant::BOOL: {
			attr_data["value"] = p_value;
			attr_data["type"] = "boolean";
		} break;
		case Variant::INT: {
			attr_data["value"] = p_value;
			attr_data["type"] = "integer";
		} break;
		case Variant::FLOAT: {
			attr_data["value"] = p_value;
			attr_data["type"] = "double";
		} break;
		default: {
			attr_data["value"] = p_value.stringify();
			attr_data["type"] = "string";
		} break;
	}

	plugin->call(ANDROID_SN(logSetAttribute), handle, attr_data);
}

void AndroidLog::add_attributes(const Dictionary &p_attributes) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(logAddAttributes), handle, p_attributes);
}

void AndroidLog::remove_attribute(const String &p_name) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(logRemoveAttribute), handle, p_name);
}

AndroidLog::AndroidLog() {
	// NOTE: Required for ClassDB registration and compilation to succeed.
	ERR_PRINT("This constructor is not intended for runtime use.");
}

AndroidLog::AndroidLog(util::ObjectHandle<Object> p_android_plugin, int32_t p_handle) :
		_android_plugin(p_android_plugin), handle(p_handle) {
}

AndroidLog::~AndroidLog() {
	if (Object *plugin = _android_plugin.get(); plugin && !is_borrowed) {
		plugin->call(ANDROID_SN(releaseLog), handle);
	}
}

} //namespace sentry::android
