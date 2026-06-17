#pragma once

#include "sentry/level.h"
#include "sentry/log_level.h"

#include <godot_cpp/classes/logger.hpp>
#include <godot_cpp/variant/string.hpp>

namespace sentry {

using GodotErrorType = godot::Logger::ErrorType;

// Enum used with bitwise operations to represent the set of Godot logger events that the Sentry logger should capture.
enum GodotLoggerEventMask {
	MASK_NONE = 0,
	MASK_ERROR = (1 << int(GodotErrorType::ERROR_TYPE_ERROR)),
	MASK_WARNING = (1 << int(GodotErrorType::ERROR_TYPE_WARNING)),
	MASK_SCRIPT = (1 << int(GodotErrorType::ERROR_TYPE_SCRIPT)),
	MASK_SHADER = (1 << int(GodotErrorType::ERROR_TYPE_SHADER)),
	// Bits 4-6 are reserved for any additional Godot error types that may be added in the future.
	MASK_MESSAGE = 1 << 7,
	MASK_ALL = MASK_ERROR | MASK_WARNING | MASK_SCRIPT | MASK_SHADER | MASK_MESSAGE,
};

// Used for exporting as PropertyInfo.
_FORCE_INLINE_ godot::String GODOT_ERROR_MASK_EXPORT_STRING() {
	return godot::vformat("Error:%d,Warning:%d,Script:%d,Shader:%d,Message:%d",
			int(MASK_ERROR), int(MASK_WARNING), int(MASK_SCRIPT), int(MASK_SHADER), int(MASK_MESSAGE));
}

// Used for exporting `godot_logger.event_mask` as PropertyInfo.
// MASK_MESSAGE is omitted because log messages should not be captured as Sentry events.
_FORCE_INLINE_ godot::String GODOT_ERROR_MASK_EXPORT_STRING_FOR_EVENTS() {
	return godot::vformat("Error:%d,Warning:%d,Script:%d,Shader:%d",
			int(MASK_ERROR), int(MASK_WARNING), int(MASK_SCRIPT), int(MASK_SHADER));
}

_FORCE_INLINE_ Level get_sentry_level_for_godot_error_type(GodotErrorType p_error_type) { return p_error_type == GodotErrorType::ERROR_TYPE_WARNING ? LEVEL_WARNING : LEVEL_ERROR; }
_FORCE_INLINE_ LogLevel get_sentry_log_level_for_godot_error_type(GodotErrorType p_error_type) { return p_error_type == GodotErrorType::ERROR_TYPE_WARNING ? LOG_LEVEL_WARN : LOG_LEVEL_ERROR; }
_FORCE_INLINE_ GodotLoggerEventMask godot_error_type_as_mask(GodotErrorType p_error_type) { return (GodotLoggerEventMask)(1 << int(p_error_type)); }

} //namespace sentry
