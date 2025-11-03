#include "sentry_logger.h"

#include "sentry/sentry_log.h" // Needed for VariantCaster<LogLevel>
#include "sentry/sentry_sdk.h"

namespace sentry {

void SentryLogger::log(LogLevel p_level, const String &p_body, const Dictionary &p_params) {
	String body = p_body;
	Dictionary attributes;
	if (!p_params.is_empty()) {
		attributes["sentry.message.template"] = p_body;
		for (const Variant &key : p_params.keys()) {
			String str_key = key.stringify();
			String param_key = "sentry.message.parameter." + str_key;
			attributes[param_key] = p_params[key];
		}
		body = p_body.format(p_params);
	}
	INTERNAL_SDK()->log(p_level, body, attributes);
}

void SentryLogger::trace(const String &p_body, const Dictionary &p_params) {
	log(LOG_LEVEL_TRACE, p_body, p_params);
}

void SentryLogger::debug(const String &p_body, const Dictionary &p_params) {
	log(LOG_LEVEL_DEBUG, p_body, p_params);
}

void SentryLogger::info(const String &p_body, const Dictionary &p_params) {
	log(LOG_LEVEL_INFO, p_body, p_params);
}

void SentryLogger::warn(const String &p_body, const Dictionary &p_params) {
	log(LOG_LEVEL_WARN, p_body, p_params);
}

void SentryLogger::error(const String &p_body, const Dictionary &p_params) {
	log(LOG_LEVEL_ERROR, p_body, p_params);
}

void SentryLogger::fatal(const String &p_body, const Dictionary &p_params) {
	log(LOG_LEVEL_FATAL, p_body, p_params);
}

void SentryLogger::_bind_methods() {
	ClassDB::bind_method(D_METHOD("log", "level", "body", "parameters"), &SentryLogger::log, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("trace", "body", "parameters"), &SentryLogger::trace, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("debug", "body", "parameters"), &SentryLogger::debug, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("info", "body", "parameters"), &SentryLogger::info, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("warn", "body", "parameters"), &SentryLogger::warn, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("error", "body", "parameters"), &SentryLogger::error, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("fatal", "body", "parameters"), &SentryLogger::fatal, DEFVAL(Dictionary()));
}

SentryLogger::SentryLogger() {}

} //namespace sentry
