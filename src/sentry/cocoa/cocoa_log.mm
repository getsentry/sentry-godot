#include "cocoa_log.h"

#include "cocoa_util.h"

namespace {

SentryLogAttribute *_as_attribute(const Variant &p_variant) {
	switch (p_variant.get_type()) {
		case Variant::BOOL:
			return [[SentryLogAttribute alloc] initWithBoolean:(bool)p_variant];
		case Variant::INT:
			return [[SentryLogAttribute alloc] initWithInteger:(int64_t)p_variant];
		case Variant::FLOAT:
			return [[SentryLogAttribute alloc] initWithDouble:(double)p_variant];
		default:
			return [[SentryLogAttribute alloc] initWithString:sentry::cocoa::string_to_objc(p_variant)];
	}
}

sentry::LogLevel _log_level_from_objc(SentryLogLevel p_level) {
	switch (p_level) {
		case SentryLogLevelTrace:
			return sentry::LOG_LEVEL_TRACE;
		case SentryLogLevelDebug:
			return sentry::LOG_LEVEL_DEBUG;
		case SentryLogLevelInfo:
			return sentry::LOG_LEVEL_INFO;
		case SentryLogLevelWarn:
			return sentry::LOG_LEVEL_WARN;
		case SentryLogLevelError:
			return sentry::LOG_LEVEL_ERROR;
		case SentryLogLevelFatal:
			return sentry::LOG_LEVEL_FATAL;
		default:
			return sentry::LOG_LEVEL_INFO;
	}
}

SentryLogLevel _log_level_to_objc(sentry::LogLevel p_level) {
	switch (p_level) {
		case sentry::LOG_LEVEL_TRACE:
			return SentryLogLevelTrace;
		case sentry::LOG_LEVEL_DEBUG:
			return SentryLogLevelDebug;
		case sentry::LOG_LEVEL_INFO:
			return SentryLogLevelInfo;
		case sentry::LOG_LEVEL_WARN:
			return SentryLogLevelWarn;
		case sentry::LOG_LEVEL_ERROR:
			return SentryLogLevelError;
		case sentry::LOG_LEVEL_FATAL:
			return SentryLogLevelFatal;
		default:
			return SentryLogLevelInfo;
	}
}

} // unnamed namespace

namespace sentry::cocoa {

LogLevel CocoaLog::get_level() const {
	return _log_level_from_objc(cocoa_log.level);
}

void CocoaLog::set_level(LogLevel p_level) {
	cocoa_log.level = _log_level_to_objc(p_level);
}

String CocoaLog::get_body() const {
	return string_from_objc(cocoa_log.body);
}

void CocoaLog::set_body(const String &p_body) {
	cocoa_log.body = string_to_objc(p_body);
}

Variant CocoaLog::get_attribute(const String &p_name) const {
	SentryLogAttribute *attribute = cocoa_log.attributes[string_to_objc(p_name)];
	if (!attribute) {
		return Variant();
	}

	return variant_from_objc(attribute.value);
}

void CocoaLog::set_attribute(const String &p_name, const Variant &p_value) {
	NSMutableDictionary *mut_attributes = [cocoa_log.attributes mutableCopy];
	[mut_attributes setObject:_as_attribute(p_value) forKey:string_to_objc(p_name)];
	cocoa_log.attributes = mut_attributes;
}

void CocoaLog::add_attributes(const Dictionary &p_attributes) {
	NSMutableDictionary *mut_attributes = [cocoa_log.attributes mutableCopy];
	const Array &keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		const String &key = keys[i].stringify();
		const Variant &value = p_attributes[key];
		[mut_attributes setObject:_as_attribute(value) forKey:string_to_objc(key)];
	}
	cocoa_log.attributes = mut_attributes;
}

void CocoaLog::remove_attribute(const String &p_name) {
	NSMutableDictionary *mut_attributes = [cocoa_log.attributes mutableCopy];
	[mut_attributes removeObjectForKey:string_to_objc(p_name)];
	cocoa_log.attributes = mut_attributes;
}

CocoaLog::CocoaLog() :
		cocoa_log(nil) {
	// NOTE: Required for ClassDB registration and compilation to succeed.
	ERR_PRINT("This constructor is not intended for runtime use.");
}

CocoaLog::CocoaLog(objc::SentryLog *p_log) :
		cocoa_log(p_log) {
}

CocoaLog::~CocoaLog() {
}

} // namespace sentry::cocoa
