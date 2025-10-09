#include "sentry_log.h"

#include "sentry/sentry_logger.h" // For LogLevel VariantCaster
#include "sentry/util/simple_bind.h"

namespace sentry {

void SentryLog::_bind_methods() {
	BIND_PROPERTY(SentryLog, sentry::make_log_level_enum_property("level"), set_level, get_level);
	BIND_PROPERTY(SentryLog, PropertyInfo(Variant::STRING, "body"), set_body, get_body);

	ClassDB::bind_method(D_METHOD("get_attribute", "key"), &SentryLog::get_attribute);
	ClassDB::bind_method(D_METHOD("set_attribute", "key", "value"), &SentryLog::set_attribute);
}

} //namespace sentry
