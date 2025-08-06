#ifndef COCOA_UTIL_H
#define COCOA_UTIL_H

#include "cocoa_includes.h"
#include "sentry/level.h"

#include <godot_cpp/variant/dictionary.hpp>

namespace sentry::cocoa {

_FORCE_INLINE_ objc::SentryLevel sentry_level_to_objc(sentry::Level p_level) {
	switch (p_level) {
		case sentry::Level::LEVEL_DEBUG:
			return kSentryLevelDebug;
		case sentry::Level::LEVEL_INFO:
			return kSentryLevelInfo;
		case sentry::Level::LEVEL_WARNING:
			return kSentryLevelWarning;
		case sentry::Level::LEVEL_ERROR:
			return kSentryLevelError;
		case sentry::Level::LEVEL_FATAL:
			return kSentryLevelFatal;
		default:
			return kSentryLevelError;
	}
}

_FORCE_INLINE_ sentry::Level sentry_level_from_objc(objc::SentryLevel p_level) {
	switch (p_level) {
		case kSentryLevelDebug:
			return sentry::Level::LEVEL_DEBUG;
		case kSentryLevelInfo:
			return sentry::Level::LEVEL_INFO;
		case kSentryLevelWarning:
			return sentry::Level::LEVEL_WARNING;
		case kSentryLevelError:
			return sentry::Level::LEVEL_ERROR;
		case kSentryLevelFatal:
			return sentry::Level::LEVEL_FATAL;
		default:
			return sentry::Level::LEVEL_ERROR;
	}
}

_FORCE_INLINE_ NSString *string_to_objc(const godot::String &p_str) {
	return [NSString stringWithUTF8String:p_str.utf8()];
}

_FORCE_INLINE_ NSString *string_to_objc_or_nil_if_empty(const godot::String &p_str) {
	return p_str.is_empty() ? nil : [NSString stringWithUTF8String:p_str.utf8()];
}

_FORCE_INLINE_ godot::String string_from_objc(NSString *p_str) {
	return p_str ? godot::String::utf8([p_str UTF8String]) : godot::String();
}

_FORCE_INLINE_ NSNumber *int_to_objc(int p_num) {
	return [NSNumber numberWithInt:p_num];
}

_FORCE_INLINE_ NSNumber *uint64_to_objc(uint64_t p_num) {
	return [NSNumber numberWithUnsignedLongLong:p_num];
}

_FORCE_INLINE_ NSNumber *bool_to_objc(bool p_flag) {
	return [NSNumber numberWithBool:p_flag];
}

_FORCE_INLINE_ NSNumber *double_to_objc(double p_num) {
	return [NSNumber numberWithDouble:p_num];
}

NSObject *variant_to_objc(const godot::Variant &p_value, int p_depth = 0);
NSDictionary *dictionary_to_objc(const godot::Dictionary &p_dictionary);

} //namespace sentry::cocoa

#endif // COCOA_UTIL_H
