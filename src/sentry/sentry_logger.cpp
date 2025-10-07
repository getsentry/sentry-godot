#include "sentry_logger.h"

#include "sentry/sentry_sdk.h"

namespace sentry {

void SentryLogger::log(sentry::Level p_level, const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(p_level, p_body, p_params, p_attributes);
}

void SentryLogger::debug(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(sentry::Level::LEVEL_DEBUG, p_body, p_params, p_attributes);
}

void SentryLogger::info(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(sentry::Level::LEVEL_INFO, p_body, p_params, p_attributes);
}

void SentryLogger::warn(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(sentry::Level::LEVEL_WARNING, p_body, p_params, p_attributes);
}

void SentryLogger::error(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(sentry::Level::LEVEL_ERROR, p_body, p_params, p_attributes);
}

void SentryLogger::fatal(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	INTERNAL_SDK()->log(sentry::Level::LEVEL_FATAL, p_body, p_params, p_attributes);
}

void SentryLogger::_bind_methods() {
	ClassDB::bind_method(D_METHOD("log", "level", "body", "params", "attributes"), &SentryLogger::log);
	ClassDB::bind_method(D_METHOD("debug", "body", "params", "attributes"), &SentryLogger::debug);
	ClassDB::bind_method(D_METHOD("info", "body", "params", "attributes"), &SentryLogger::info);
	ClassDB::bind_method(D_METHOD("warn", "body", "params", "attributes"), &SentryLogger::warn);
	ClassDB::bind_method(D_METHOD("error", "body", "params", "attributes"), &SentryLogger::error);
	ClassDB::bind_method(D_METHOD("fatal", "body", "params", "attributes"), &SentryLogger::fatal);
}

SentryLogger::SentryLogger() {}

} //namespace sentry
