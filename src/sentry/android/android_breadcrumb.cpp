#include "android_breadcrumb.h"

#include "android_string_names.h"
#include "android_util.h"

namespace sentry::android {

void AndroidBreadcrumb::set_message(const String &p_message) {
	ENSURE_ANDROID_PLUGIN(plugin);
	plugin->call(ANDROID_SN(breadcrumbSetMessage), handle, p_message);
}

String AndroidBreadcrumb::get_message() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, String());
	return plugin->call(ANDROID_SN(breadcrumbGetMessage), handle);
}

void AndroidBreadcrumb::set_category(const String &p_category) {
	ENSURE_ANDROID_PLUGIN(plugin);
	plugin->call(ANDROID_SN(breadcrumbSetCategory), handle, p_category);
}

String AndroidBreadcrumb::get_category() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, String());
	return plugin->call(ANDROID_SN(breadcrumbGetCategory), handle);
}

void AndroidBreadcrumb::set_level(sentry::Level p_level) {
	ENSURE_ANDROID_PLUGIN(plugin);
	plugin->call(ANDROID_SN(breadcrumbSetLevel), handle, p_level);
}

sentry::Level AndroidBreadcrumb::get_level() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, sentry::Level::LEVEL_INFO);
	Variant result = plugin->call(ANDROID_SN(breadcrumbGetLevel), handle);
	ERR_FAIL_COND_V(result.get_type() != Variant::INT, sentry::Level::LEVEL_INFO);
	return sentry::int_to_level(result);
}

void AndroidBreadcrumb::set_type(const String &p_type) {
	ENSURE_ANDROID_PLUGIN(plugin);
	plugin->call(ANDROID_SN(breadcrumbSetType), handle, p_type);
}

String AndroidBreadcrumb::get_type() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, String());
	return plugin->call(ANDROID_SN(breadcrumbGetType), handle);
}

void AndroidBreadcrumb::set_data(const Dictionary &p_data) {
	ENSURE_ANDROID_PLUGIN(plugin);
	plugin->call(ANDROID_SN(breadcrumbSetData), handle, p_data);
}

Ref<SentryTimestamp> AndroidBreadcrumb::get_timestamp() {
	ENSURE_ANDROID_PLUGIN_V(plugin, nullptr);
	int64_t micros = plugin->call(ANDROID_SN(breadcrumbGetTimestamp), handle);
	return SentryTimestamp::from_microseconds_since_unix_epoch(micros);
}

AndroidBreadcrumb::AndroidBreadcrumb(util::ObjectHandle<Object> p_android_plugin, int32_t p_breadcrumb_handle) :
		_android_plugin(p_android_plugin), handle(p_breadcrumb_handle) {
}

AndroidBreadcrumb::~AndroidBreadcrumb() {
	if (Object *plugin = _android_plugin.get()) {
		plugin->call(ANDROID_SN(releaseBreadcrumb), handle);
	}
}

} //namespace sentry::android
