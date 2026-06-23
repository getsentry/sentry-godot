#pragma once

#include "cocoa_includes.h"
#include "sentry/level.h"

#include <godot_cpp/variant/dictionary.hpp>

namespace sentry::cocoa {

_FORCE_INLINE_ SentryObjCLevel sentry_level_to_objc(sentry::Level p_level) {
	switch (p_level) {
		case sentry::Level::LEVEL_DEBUG:
			return SentryObjCLevelDebug;
		case sentry::Level::LEVEL_INFO:
			return SentryObjCLevelInfo;
		case sentry::Level::LEVEL_WARNING:
			return SentryObjCLevelWarning;
		case sentry::Level::LEVEL_ERROR:
			return SentryObjCLevelError;
		case sentry::Level::LEVEL_FATAL:
			return SentryObjCLevelFatal;
		default:
			return SentryObjCLevelError;
	}
}

_FORCE_INLINE_ sentry::Level sentry_level_from_objc(SentryObjCLevel p_level) {
	switch (p_level) {
		case SentryObjCLevelDebug:
			return sentry::Level::LEVEL_DEBUG;
		case SentryObjCLevelInfo:
			return sentry::Level::LEVEL_INFO;
		case SentryObjCLevelWarning:
			return sentry::Level::LEVEL_WARNING;
		case SentryObjCLevelError:
			return sentry::Level::LEVEL_ERROR;
		case SentryObjCLevelFatal:
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
godot::Variant variant_from_objc(const NSObject *p_object);

NSDictionary *dictionary_to_objc(const godot::Dictionary &p_dictionary);
NSArray<NSString *> *string_array_to_objc(const godot::PackedStringArray &p_array);

_FORCE_INLINE_ SentryObjCAttribute *variant_to_attribute(const godot::Variant &p_variant) {
	switch (p_variant.get_type()) {
		case godot::Variant::BOOL:
			return [[SentryObjCAttribute alloc] initWithBoolean:p_variant.operator bool()];
		case godot::Variant::INT:
			return [[SentryObjCAttribute alloc] initWithInteger:p_variant.operator int64_t()];
		case godot::Variant::FLOAT:
			return [[SentryObjCAttribute alloc] initWithDouble:p_variant.operator double()];
		case godot::Variant::STRING:
			return [[SentryObjCAttribute alloc] initWithString:string_to_objc(p_variant.operator godot::String())];
		default:
			return [[SentryObjCAttribute alloc] initWithString:string_to_objc(p_variant.stringify())];
	}
}

_FORCE_INLINE_ SentryObjCAttributeContent *variant_to_attribute_content(const godot::Variant &p_variant) {
	switch (p_variant.get_type()) {
		case godot::Variant::BOOL:
			return [SentryObjCAttributeContent boolean:p_variant.operator bool()];
		case godot::Variant::INT:
			return [SentryObjCAttributeContent integer:(NSInteger)p_variant.operator int64_t()];
		case godot::Variant::FLOAT:
			return [SentryObjCAttributeContent double:p_variant.operator double()];
		case godot::Variant::STRING:
			return [SentryObjCAttributeContent string:string_to_objc(p_variant.operator godot::String())];
		default:
			return [SentryObjCAttributeContent string:string_to_objc(p_variant.stringify())];
	}
}

} //namespace sentry::cocoa
