#include "sentry_breadcrumb.h"

#include "sentry/util/simple_bind.h"
#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

void SentryBreadcrumb::_bind_methods() {
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, message);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, category);
	BIND_PROPERTY(SentryBreadcrumb, sentry::make_level_enum_property("level"), set_level, get_level);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, type);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::DICTIONARY, data);
}
