#include "sentry_logger.h"

#include "sentry/sentry_log.h" // Needed for VariantCaster<LogLevel>
#include "sentry/sentry_sdk.h"

namespace sentry {

void SentryLogger::log(LogLevel p_level, const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	String body = p_body;
	Dictionary attributes;
	attributes.merge(p_attributes);
	if (!p_params.is_empty()) {
		attributes["sentry.message.template"] = p_body;
		for (int i = 0; i < p_params.size(); i++) {
			String attr_key = "sentry.message.parameter." + itos(i);
			attributes[attr_key] = p_params[i];
		}
		body = p_body % p_params;
	}
	INTERNAL_SDK()->log(p_level, body, attributes);
}

void SentryLogger::trace(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	log(LOG_LEVEL_TRACE, p_body, p_params, p_attributes);
}

void SentryLogger::debug(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	log(LOG_LEVEL_DEBUG, p_body, p_params, p_attributes);
}

void SentryLogger::info(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	log(LOG_LEVEL_INFO, p_body, p_params, p_attributes);
}

void SentryLogger::warn(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	log(LOG_LEVEL_WARN, p_body, p_params, p_attributes);
}

void SentryLogger::error(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	log(LOG_LEVEL_ERROR, p_body, p_params, p_attributes);
}

void SentryLogger::fatal(const String &p_body, const Array &p_params, const Dictionary &p_attributes) {
	log(LOG_LEVEL_FATAL, p_body, p_params, p_attributes);
}

void SentryLogger::_bind_methods() {
	ClassDB::bind_method(D_METHOD("log", "level", "body", "parameters", "attributes"), &SentryLogger::log, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("trace", "body", "parameters", "attributes"), &SentryLogger::trace, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("debug", "body", "parameters", "attributes"), &SentryLogger::debug, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("info", "body", "parameters", "attributes"), &SentryLogger::info, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("warn", "body", "parameters", "attributes"), &SentryLogger::warn, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("error", "body", "parameters", "attributes"), &SentryLogger::error, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("fatal", "body", "parameters", "attributes"), &SentryLogger::fatal, DEFVAL(Array()), DEFVAL(Dictionary()));
}

SentryLogger::SentryLogger() {}

} //namespace sentry
