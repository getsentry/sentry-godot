#include "sentry_breadcrumb.h"

#include "sentry/util/simple_bind.h"
#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

namespace sentry {

Ref<SentryBreadcrumb> SentryBreadcrumb::create(const String &p_message) {
	ERR_FAIL_NULL_V(SentrySDK::get_singleton(), nullptr);
	auto internal_sdk = SentrySDK::get_singleton()->get_internal_sdk();
	Ref<SentryBreadcrumb> instance = internal_sdk->create_breadcrumb();
	return instance;
}

void SentryBreadcrumb::_bind_methods() {
	ClassDB::bind_static_method("SentryBreadcrumb", D_METHOD("create", "message"), &SentryBreadcrumb::create);

	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, message);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, category);
	BIND_PROPERTY(SentryBreadcrumb, sentry::make_level_enum_property("level"), set_level, get_level);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, type);

	ClassDB::bind_method(D_METHOD("set_data", "data"), &SentryBreadcrumb::set_data);
}

} //namespace sentry
