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

_FORCE_INLINE_ NSString *string_to_objc(const godot::String &p_str) {
	return [NSString stringWithUTF8String:p_str.utf8()];
}

_FORCE_INLINE_ godot::String string_from_objc(NSString *p_str) {
	return p_str ? godot::String::utf8([p_str UTF8String]) : godot::String();
}

_FORCE_INLINE_ NSNumber *double_to_objc(double p_num) {
	return [NSNumber numberWithDouble:p_num];
}

NSObject *variant_to_objc(const godot::Variant &p_value);
NSDictionary *dictionary_to_objc(const godot::Dictionary &p_dictionary);

} //namespace sentry::cocoa

#endif // COCOA_UTIL_H
