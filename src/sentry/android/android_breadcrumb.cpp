#include "android_breadcrumb.h"

#include "android_string_names.h"

namespace sentry::android {

void AndroidBreadcrumb::set_message(const String &p_message) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(breadcrumbSetMessage), handle);
}

String AndroidBreadcrumb::get_message() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(breadcrumbGetMessage), handle);
}

void AndroidBreadcrumb::set_category(const String &p_category) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(breadcrumbSetCategory), handle, p_category);
}

String AndroidBreadcrumb::get_category() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(breadcrumbGetCategory), handle);
}

void AndroidBreadcrumb::set_level(sentry::Level p_level) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(breadcrumbSetLevel), handle, p_level);
}

sentry::Level AndroidBreadcrumb::get_level() const {
	ERR_FAIL_NULL_V(android_plugin, sentry::Level::LEVEL_INFO);
	Variant result = android_plugin->call(ANDROID_SN(breadcrumbGetLevel), handle);
	ERR_FAIL_COND_V(result.get_type() != Variant::INT, sentry::Level::LEVEL_INFO);
	return sentry::int_to_level(result);
}

void AndroidBreadcrumb::set_type(const String &p_type) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(breadcrumbSetType), handle, p_type);
}

String AndroidBreadcrumb::get_type() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(breadcrumbGetType), handle);
}

void AndroidBreadcrumb::set_data(const Dictionary &p_data) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(breadcrumbSetData), handle, p_data);
}

Ref<SentryTimestamp> AndroidBreadcrumb::get_timestamp() {
	// not implemented
}

AndroidBreadcrumb::AndroidBreadcrumb(Object *android_plugin, int32_t p_breadcrumb_handle) :
		android_plugin(android_plugin), handle(p_breadcrumb_handle) {
	ERR_FAIL_NULL(android_plugin);
}

AndroidBreadcrumb::~AndroidBreadcrumb() {
	if (android_plugin) {
		android_plugin->call(ANDROID_SN(releaseBreadcrumb), handle);
	}
}

} //namespace sentry::android
