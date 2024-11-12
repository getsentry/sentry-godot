#include "native_util.h"

namespace sentry::native {

sentry_value_t variant_to_sentry_value(const Variant &p_variant) {
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

sentry_value_t strings_to_sentry_list(const PackedStringArray &p_strings) {
	sentry_value_t sentry_list = sentry_value_new_list();
	for (int i = 0; i < p_strings.size(); i++) {
		sentry_value_append(sentry_list, sentry_value_new_string(p_strings[i].utf8()));
	}
	return sentry_list;
}

String make_uuid() {
	sentry_uuid_t uuid = sentry_uuid_new_v4();
	char cstr[37];
	sentry_uuid_as_string(&uuid, cstr);
	return String(cstr);
}

} // namespace sentry::native
