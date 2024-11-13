#ifndef LOGGER_ERROR_TYPES_H
#define LOGGER_ERROR_TYPES_H

#include "godot_cpp/core/defs.hpp"
#include "level.h"

#include <godot_cpp/variant/string.hpp>

namespace sentry {

// Godot Engine error types that we can detect.
// As defined in: https://github.com/godotengine/godot/blob/04692d83cb8f61002f18ea1d954df8c558ee84f7/core/io/logger.h#L51-L56
enum class GodotErrorType {
	ERROR_TYPE_ERROR,
	ERROR_TYPE_WARNING,
	ERROR_TYPE_SCRIPT,
	ERROR_TYPE_SHADER,
};

// Enum used with bitwise operations to represent the set of Godot error types that the Sentry logger should capture.
enum class GodotErrorMask {
	NONE = 0,
	ERROR = 1 << int(GodotErrorType::ERROR_TYPE_ERROR),
	WARNING = 1 << int(GodotErrorType::ERROR_TYPE_WARNING),
	SCRIPT = 1 << int(GodotErrorType::ERROR_TYPE_SCRIPT),
	SHADER = 1 << int(GodotErrorType::ERROR_TYPE_SHADER),
	ALL = ERROR | WARNING | SCRIPT | SHADER,
	ALL_EXCEPT_WARNING = ERROR | SCRIPT | SHADER,
};

// Used for exporting as PropertyInfo.
const String GODOT_ERROR_MASK_EXPORT_STRING{ "Error,Warning,Script,Shader" };

_FORCE_INLINE_ Level get_sentry_level_for_godot_error_type(GodotErrorType p_error_type) { return p_error_type == GodotErrorType::ERROR_TYPE_WARNING ? LEVEL_WARNING : LEVEL_ERROR; }
_FORCE_INLINE_ int godot_error_type_as_mask(GodotErrorType p_error_type) { return 1 << int(p_error_type); }

} //namespace sentry

#endif // LOGGER_ERROR_TYPES_H
