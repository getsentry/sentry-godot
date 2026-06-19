#include "cocoa_log.h"

#include "cocoa_util.h"

namespace {

sentry::LogLevel _log_level_from_objc(SentryObjCLogLevel p_level) {
	switch (p_level) {
		case SentryObjCLogLevelTrace:
			return sentry::LOG_LEVEL_TRACE;
		case SentryObjCLogLevelDebug:
			return sentry::LOG_LEVEL_DEBUG;
		case SentryObjCLogLevelInfo:
			return sentry::LOG_LEVEL_INFO;
		case SentryObjCLogLevelWarn:
			return sentry::LOG_LEVEL_WARN;
		case SentryObjCLogLevelError:
			return sentry::LOG_LEVEL_ERROR;
		case SentryObjCLogLevelFatal:
			return sentry::LOG_LEVEL_FATAL;
		default:
			return sentry::LOG_LEVEL_INFO;
	}
}

SentryObjCLogLevel _log_level_to_objc(sentry::LogLevel p_level) {
	switch (p_level) {
		case sentry::LOG_LEVEL_TRACE:
			return SentryObjCLogLevelTrace;
		case sentry::LOG_LEVEL_DEBUG:
			return SentryObjCLogLevelDebug;
		case sentry::LOG_LEVEL_INFO:
			return SentryObjCLogLevelInfo;
		case sentry::LOG_LEVEL_WARN:
			return SentryObjCLogLevelWarn;
		case sentry::LOG_LEVEL_ERROR:
			return SentryObjCLogLevelError;
		case sentry::LOG_LEVEL_FATAL:
			return SentryObjCLogLevelFatal;
		default:
			return SentryObjCLogLevelInfo;
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
	SentryObjCAttribute *attribute = cocoa_log.attributes[string_to_objc(p_name)];
	if (!attribute) {
		return Variant();
	}

	return variant_from_objc(attribute.value);
}

void CocoaLog::set_attribute(const String &p_name, const Variant &p_value) {
	NSMutableDictionary *mut_attributes = [cocoa_log.attributes mutableCopy] ?: [NSMutableDictionary dictionary];
	[mut_attributes setObject:variant_to_attribute(p_value) forKey:string_to_objc(p_name)];
	cocoa_log.attributes = mut_attributes;
}

void CocoaLog::add_attributes(const Dictionary &p_attributes) {
	if (p_attributes.is_empty()) {
		return;
	}

	NSMutableDictionary *mut_attributes = [cocoa_log.attributes mutableCopy] ?: [NSMutableDictionary dictionary];
	const Array &keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		const Variant &key = keys[i];
		const Variant &value = p_attributes[key];
		[mut_attributes setObject:variant_to_attribute(value) forKey:string_to_objc(key.stringify())];
	}
	cocoa_log.attributes = mut_attributes;
}

void CocoaLog::remove_attribute(const String &p_name) {
	NSMutableDictionary *mut_attributes = [cocoa_log.attributes mutableCopy];
	[mut_attributes removeObjectForKey:string_to_objc(p_name)];
	cocoa_log.attributes = mut_attributes.count > 0 ? mut_attributes : nil;
}

CocoaLog::CocoaLog() :
		cocoa_log(nil) {
	// NOTE: Required for ClassDB registration and compilation to succeed.
	ERR_PRINT("This constructor is not intended for runtime use.");
}

CocoaLog::CocoaLog(SentryObjCLog *p_log) :
		cocoa_log(p_log) {
}

CocoaLog::~CocoaLog() {
}

} // namespace sentry::cocoa
