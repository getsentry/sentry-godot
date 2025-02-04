#include "sentry_breadcrumb.h"

#include "sentry/simple_bind.h"
#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

void SentryBreadcrumb::_bind_methods() {
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, message);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, category);
	BIND_PROPERTY(SentryBreadcrumb, sentry::make_level_enum_property("level"), set_level, get_level);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, type);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, timestamp);

	ClassDB::bind_method(D_METHOD("get_data"), &SentryBreadcrumb::get_data);
	ClassDB::bind_method(D_METHOD("set_data", "data"), &SentryBreadcrumb::set_data);
}
