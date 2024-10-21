#include "sentry_util.h"

#include <sentry.h>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

sentry_value_t SentryUtil::variant_to_sentry_value(const godot::Variant &p_variant) {
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
				sentry_value_set_by_key(sentry_dic, key.utf8(), variant_to_sentry_value(dic[key]));
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
					sentry_value_append(sentry_list, variant_to_sentry_value(item));
				}
			} while (!oob);
			return sentry_list;
		} break;
		default: {
			return sentry_value_new_string(p_variant.stringify().utf8());
		} break;
	}
}

godot::CharString SentryUtil::get_screen_orientation_cstring(int32_t p_screen) {
	ERR_FAIL_NULL_V(DisplayServer::get_singleton(), "");
	switch (DisplayServer::get_singleton()->screen_get_orientation(p_screen)) {
		case DisplayServer::SCREEN_LANDSCAPE: {
			return "Landscape";
		} break;
		case DisplayServer::SCREEN_PORTRAIT: {
			return "Portrait";
		} break;
		case DisplayServer::SCREEN_REVERSE_LANDSCAPE: {
			return "Landscape (reverse)";
		} break;
		case DisplayServer::SCREEN_REVERSE_PORTRAIT: {
			return "Portrait (reverse)";
		} break;
		case DisplayServer::SCREEN_SENSOR_LANDSCAPE: {
			return "Landscape (defined by sensor)";
		} break;
		case DisplayServer::SCREEN_SENSOR_PORTRAIT: {
			return "Portrait (defined by sensor)";
		} break;
		case DisplayServer::SCREEN_SENSOR: {
			return "Defined by sensor";
		} break;
		default: {
			return "";
		} break;
	}
}

void SentryUtil::sentry_event_set_context(sentry_value_t p_event, const char *p_context_name, sentry_value_t p_context) {
	ERR_FAIL_COND(sentry_value_get_type(p_event) != SENTRY_VALUE_TYPE_OBJECT);
	ERR_FAIL_COND(sentry_value_get_type(p_context) != SENTRY_VALUE_TYPE_OBJECT);
	ERR_FAIL_COND(strlen(p_context_name) == 0);

	sentry_value_t contexts = sentry_value_get_by_key(p_event, "contexts");
	if (sentry_value_is_null(contexts)) {
		contexts = sentry_value_new_object();
		sentry_value_set_by_key(p_event, "contexts", contexts);
	}
	sentry_value_set_by_key(contexts, p_context_name, p_context);
}
