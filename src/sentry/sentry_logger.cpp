#include "sentry_logger.h"

#include "sentry/sentry_log.h" // Needed for VariantCaster<LogLevel>
#include "sentry/sentry_sdk.h"

namespace sentry {

void SentryLogger::log(LogLevel p_level, const String &p_body) {
	INTERNAL_SDK()->log(p_level, p_body);
}

void SentryLogger::trace(const String &p_body) {
	INTERNAL_SDK()->log(LOG_LEVEL_TRACE, p_body);
}

void SentryLogger::debug(const String &p_body) {
	INTERNAL_SDK()->log(LOG_LEVEL_DEBUG, p_body);
}

void SentryLogger::info(const String &p_body) {
	INTERNAL_SDK()->log(LOG_LEVEL_INFO, p_body);
}

void SentryLogger::warn(const String &p_body) {
	INTERNAL_SDK()->log(LOG_LEVEL_WARN, p_body);
}

void SentryLogger::error(const String &p_body) {
	INTERNAL_SDK()->log(LOG_LEVEL_ERROR, p_body);
}

void SentryLogger::fatal(const String &p_body) {
	INTERNAL_SDK()->log(LOG_LEVEL_FATAL, p_body);
}

void SentryLogger::_bind_methods() {
	ClassDB::bind_method(D_METHOD("log", "level", "body"), &SentryLogger::log);
	ClassDB::bind_method(D_METHOD("trace", "body"), &SentryLogger::trace);
	ClassDB::bind_method(D_METHOD("debug", "body"), &SentryLogger::debug);
	ClassDB::bind_method(D_METHOD("info", "body"), &SentryLogger::info);
	ClassDB::bind_method(D_METHOD("warn", "body"), &SentryLogger::warn);
	ClassDB::bind_method(D_METHOD("error", "body"), &SentryLogger::error);
	ClassDB::bind_method(D_METHOD("fatal", "body"), &SentryLogger::fatal);
}

SentryLogger::SentryLogger() {}

} //namespace sentry
