#include "sentry_breadcrumb.h"

#include "sentry/util/simple_bind.h"
#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

namespace {

using namespace sentry;

inline Ref<SentryBreadcrumb> _create_breadcrumb() {
	ERR_FAIL_NULL_V(SentrySDK::get_singleton(), nullptr);
	auto internal_sdk = SentrySDK::get_singleton()->get_internal_sdk();
	Ref<SentryBreadcrumb> instance = internal_sdk->create_breadcrumb();
	ERR_FAIL_COND_V(instance.is_null(), nullptr);
	return instance;
}

} // unnamed namespace

namespace sentry {

Ref<SentryBreadcrumb> SentryBreadcrumb::debug(const String &p_message) {
	Ref<SentryBreadcrumb> instance = _create_breadcrumb();
	instance->set_type("debug");
	instance->set_message(p_message);
	instance->set_level(sentry::Level::LEVEL_DEBUG);
	return instance;
}

Ref<SentryBreadcrumb> SentryBreadcrumb::info(const String &p_message) {
	Ref<SentryBreadcrumb> instance = _create_breadcrumb();
	instance->set_type("info");
	instance->set_message(p_message);
	instance->set_level(sentry::Level::LEVEL_INFO);
	return instance;
}

Ref<SentryBreadcrumb> SentryBreadcrumb::error(const String &p_message) {
	Ref<SentryBreadcrumb> instance = _create_breadcrumb();
	instance->set_type("error");
	instance->set_message(p_message);
	instance->set_level(sentry::Level::LEVEL_ERROR);
	return instance;
}

Ref<SentryBreadcrumb> SentryBreadcrumb::query(const String &p_message) {
	Ref<SentryBreadcrumb> instance = _create_breadcrumb();
	instance->set_type("query");
	instance->set_message(p_message);
	instance->set_level(sentry::Level::LEVEL_INFO);
	return instance;
}

void SentryBreadcrumb::_bind_methods() {
	ClassDB::bind_static_method("SentryBreadcrumb", D_METHOD("debug", "message"), &SentryBreadcrumb::debug);
	ClassDB::bind_static_method("SentryBreadcrumb", D_METHOD("info", "message"), &SentryBreadcrumb::info);
	ClassDB::bind_static_method("SentryBreadcrumb", D_METHOD("error", "message"), &SentryBreadcrumb::error);
	ClassDB::bind_static_method("SentryBreadcrumb", D_METHOD("query", "message"), &SentryBreadcrumb::query);

	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, message);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, category);
	BIND_PROPERTY(SentryBreadcrumb, sentry::make_level_enum_property("level"), set_level, get_level);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, type);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::DICTIONARY, data);
}

} //namespace sentry
