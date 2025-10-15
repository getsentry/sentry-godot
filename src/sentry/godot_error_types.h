#ifndef LOGGER_ERROR_TYPES_H
#define LOGGER_ERROR_TYPES_H

#include "sentry/level.h"
#include "sentry/log_level.h"

#include <godot_cpp/classes/logger.hpp>
#include <godot_cpp/variant/string.hpp>

namespace sentry {

using GodotErrorType = godot::Logger::ErrorType;

// Enum used with bitwise operations to represent the set of Godot error types that the Sentry logger should capture.
enum GodotErrorMask {
	MASK_NONE = 0,
	MASK_ERROR = (1 << int(GodotErrorType::ERROR_TYPE_ERROR)),
	MASK_WARNING = (1 << int(GodotErrorType::ERROR_TYPE_WARNING)),
	MASK_SCRIPT = (1 << int(GodotErrorType::ERROR_TYPE_SCRIPT)),
	MASK_SHADER = (1 << int(GodotErrorType::ERROR_TYPE_SHADER)),
	MASK_ALL = (int(MASK_ERROR) | int(MASK_WARNING) | int(MASK_SCRIPT) | int(MASK_SHADER)),
	MASK_ALL_EXCEPT_WARNING = (int(MASK_ERROR) | int(MASK_SCRIPT) | int(MASK_SHADER)),
};

// Used for exporting as PropertyInfo.
_FORCE_INLINE_ godot::String GODOT_ERROR_MASK_EXPORT_STRING() { return "Error,Warning,Script,Shader"; }

_FORCE_INLINE_ Level get_sentry_level_for_godot_error_type(GodotErrorType p_error_type) { return p_error_type == GodotErrorType::ERROR_TYPE_WARNING ? LEVEL_WARNING : LEVEL_ERROR; }
_FORCE_INLINE_ LogLevel get_sentry_log_level_for_godot_error_type(GodotErrorType p_error_type) { return p_error_type == GodotErrorType::ERROR_TYPE_WARNING ? LOG_LEVEL_WARN : LOG_LEVEL_ERROR; }
_FORCE_INLINE_ GodotErrorMask godot_error_type_as_mask(GodotErrorType p_error_type) { return (GodotErrorMask)(1 << int(p_error_type)); }

} //namespace sentry

#endif // LOGGER_ERROR_TYPES_H
