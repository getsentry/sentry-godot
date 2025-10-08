#include "sentry_logger.h"

#include "sentry/sentry_sdk.h"

namespace sentry {

void SentryLogger::log(LogLevel p_level, const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(p_level, p_body, p_params, p_attributes);
}

void SentryLogger::trace(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(LOG_LEVEL_TRACE, p_body, p_params, p_attributes);
}

void SentryLogger::debug(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(LOG_LEVEL_DEBUG, p_body, p_params, p_attributes);
}

void SentryLogger::info(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(LOG_LEVEL_INFO, p_body, p_params, p_attributes);
}

void SentryLogger::warn(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(LOG_LEVEL_WARN, p_body, p_params, p_attributes);
}

void SentryLogger::error(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(LOG_LEVEL_ERROR, p_body, p_params, p_attributes);
}

void SentryLogger::fatal(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(LOG_LEVEL_FATAL, p_body, p_params, p_attributes);
}

void SentryLogger::_bind_methods() {
	ClassDB::bind_method(D_METHOD("log", "level", "body", "params", "attributes"), &SentryLogger::log, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("trace", "body", "params", "attributes"), &SentryLogger::trace, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("debug", "body", "params", "attributes"), &SentryLogger::debug, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("info", "body", "params", "attributes"), &SentryLogger::info, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("warn", "body", "params", "attributes"), &SentryLogger::warn, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("error", "body", "params", "attributes"), &SentryLogger::error, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("fatal", "body", "params", "attributes"), &SentryLogger::fatal, DEFVAL(Array()), DEFVAL(Dictionary()));

	BIND_ENUM_CONSTANT(LOG_LEVEL_TRACE);
	BIND_ENUM_CONSTANT(LOG_LEVEL_DEBUG);
	BIND_ENUM_CONSTANT(LOG_LEVEL_INFO);
	BIND_ENUM_CONSTANT(LOG_LEVEL_WARN);
	BIND_ENUM_CONSTANT(LOG_LEVEL_ERROR);
	BIND_ENUM_CONSTANT(LOG_LEVEL_FATAL);
}

SentryLogger::SentryLogger() {}

} //namespace sentry
