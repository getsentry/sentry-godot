#include "sentry_log.h"

#include "sentry/util/simple_bind.h"

namespace sentry {

void SentryLog::_bind_methods() {
	BIND_PROPERTY(SentryLog, sentry::make_log_level_enum_property("level"), set_level, get_level);
	BIND_PROPERTY(SentryLog, PropertyInfo(Variant::STRING, "body"), set_body, get_body);

	ClassDB::bind_method(D_METHOD("get_attribute", "name"), &SentryLog::get_attribute);
	ClassDB::bind_method(D_METHOD("set_attribute", "name", "value"), &SentryLog::set_attribute);
	ClassDB::bind_method(D_METHOD("add_attributes", "attributes"), &SentryLog::add_attributes);
	ClassDB::bind_method(D_METHOD("remove_attribute", "name"), &SentryLog::remove_attribute);

	BIND_ENUM_CONSTANT(LOG_LEVEL_TRACE);
	BIND_ENUM_CONSTANT(LOG_LEVEL_DEBUG);
	BIND_ENUM_CONSTANT(LOG_LEVEL_INFO);
	BIND_ENUM_CONSTANT(LOG_LEVEL_WARN);
	BIND_ENUM_CONSTANT(LOG_LEVEL_ERROR);
	BIND_ENUM_CONSTANT(LOG_LEVEL_FATAL);
}

} //namespace sentry
