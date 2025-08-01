#include "native_util.h"

namespace sentry::native {

sentry_value_t variant_to_sentry_value(const Variant &p_variant, int p_depth) {
	if (p_depth > 32) {
		ERR_PRINT_ONCE("Sentry: Maximum Variant conversion depth reached!");
		return sentry_value_new_null();
	}

	switch (p_variant.get_type()) {
		case Variant::Type::NIL: {
			return sentry_value_new_null();
		} break;
		case Variant::Type::BOOL: {
			return sentry_value_new_bool((bool)p_variant);
		} break;
		case Variant::Type::INT: {
			return sentry_value_new_int32((int32_t)p_variant);
		} break;
		case Variant::Type::FLOAT: {
			return sentry_value_new_double((double)p_variant);
		} break;
		case Variant::Type::STRING: {
			return sentry_value_new_string(((String)p_variant).utf8());
		} break;
		case Variant::Type::DICTIONARY: {
			Dictionary dic = p_variant;
			sentry_value_t sentry_dic = sentry_value_new_object();
			const Array &keys = dic.keys();
			for (int i = 0; i < keys.size(); i++) {
				const String &key = keys[i];
				sentry_value_set_by_key(sentry_dic, key.utf8(), variant_to_sentry_value(dic[key], p_depth + 1));
			}
			return sentry_dic;
		} break;
		case Variant::Type::ARRAY:
		case Variant::Type::PACKED_BYTE_ARRAY:
		case Variant::Type::PACKED_INT32_ARRAY:
		case Variant::Type::PACKED_INT64_ARRAY:
		case Variant::Type::PACKED_FLOAT32_ARRAY:
		case Variant::Type::PACKED_FLOAT64_ARRAY:
		case Variant::Type::PACKED_STRING_ARRAY:
		case Variant::Type::PACKED_VECTOR2_ARRAY:
		case Variant::Type::PACKED_VECTOR3_ARRAY:
		case Variant::Type::PACKED_COLOR_ARRAY:
		case Variant::Type::PACKED_VECTOR4_ARRAY: {
			bool oob = false;
			bool valid = true;
			int i = 0;
			sentry_value_t sentry_list = sentry_value_new_list();
			do {
				Variant item = p_variant.get_indexed(i++, valid, oob);
				if (valid) {
					sentry_value_append(sentry_list, variant_to_sentry_value(item, p_depth + 1));
				}
			} while (!oob);
			return sentry_list;
		} break;
		default: {
			return sentry_value_new_string(p_variant.stringify().utf8());
		} break;
	}
}

sentry_value_t strings_to_sentry_list(const PackedStringArray &p_strings) {
	sentry_value_t sentry_list = sentry_value_new_list();
	for (int i = 0; i < p_strings.size(); i++) {
		sentry_value_append(sentry_list, sentry_value_new_string(p_strings[i].utf8()));
	}
	return sentry_list;
}

sentry_level_t level_to_native(sentry::Level p_level) {
	switch (p_level) {
		case sentry::Level::LEVEL_DEBUG:
			return SENTRY_LEVEL_DEBUG;
		case sentry::Level::LEVEL_INFO:
			return SENTRY_LEVEL_INFO;
		case sentry::Level::LEVEL_WARNING:
			return SENTRY_LEVEL_WARNING;
		case sentry::Level::LEVEL_ERROR:
			return SENTRY_LEVEL_ERROR;
		case sentry::Level::LEVEL_FATAL:
			return SENTRY_LEVEL_FATAL;
		default:
			ERR_FAIL_V_MSG(SENTRY_LEVEL_ERROR, "SentrySDK: Internal error - unexpected level value. Please open an issue.");
	}
}

sentry::Level native_to_level(sentry_level_t p_native_level) {
	switch (p_native_level) {
		case SENTRY_LEVEL_DEBUG:
			return sentry::Level::LEVEL_DEBUG;
		case SENTRY_LEVEL_INFO:
			return sentry::Level::LEVEL_INFO;
		case SENTRY_LEVEL_WARNING:
			return sentry::Level::LEVEL_WARNING;
		case SENTRY_LEVEL_ERROR:
			return sentry::Level::LEVEL_ERROR;
		case SENTRY_LEVEL_FATAL:
			return sentry::Level::LEVEL_FATAL;
		default:
			ERR_FAIL_V_MSG(sentry::Level::LEVEL_ERROR, "SentrySDK: Internal error - unexpected level value. Please open an issue.");
	}
}

CharString level_to_cstring(Level level) {
	switch (level) {
		case Level::LEVEL_DEBUG:
			return "debug";
		case Level::LEVEL_INFO:
			return "info";
		case Level::LEVEL_WARNING:
			return "warning";
		case Level::LEVEL_ERROR:
			return "error";
		case Level::LEVEL_FATAL:
			return "fatal";
		default:
			ERR_FAIL_V_MSG("error", "SentrySDK: Internal error - unexpected level value. Please open an issue.");
	}
}

Level cstring_to_level(const CharString &p_cstring) {
	if (strcmp(p_cstring, "debug") == 0) {
		return Level::LEVEL_DEBUG;
	} else if (strcmp(p_cstring, "info") == 0) {
		return Level::LEVEL_INFO;
	} else if (strcmp(p_cstring, "warning") == 0) {
		return Level::LEVEL_WARNING;
	} else if (strcmp(p_cstring, "error") == 0) {
		return Level::LEVEL_ERROR;
	} else if (strcmp(p_cstring, "fatal") == 0) {
		return Level::LEVEL_FATAL;
	} else {
		ERR_FAIL_V_MSG(Level::LEVEL_ERROR, "SentrySDK: Internal error - unexpected level value. Please open an issue.");
	}
}

} // namespace sentry::native
