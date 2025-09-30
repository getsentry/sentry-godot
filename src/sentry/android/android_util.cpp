#include "android_util.h"

#include "sentry/common_defs.h"

using namespace godot;

namespace sentry::android {

Variant sanitize_variant(const Variant &p_value, int p_depth) {
	switch (p_value.get_type()) {
		case Variant::DICTIONARY: {
			if (p_depth > VARIANT_CONVERSION_MAX_DEPTH) {
				ERR_PRINT_ONCE("Sentry: Maximum Variant depth reached!");
				return Variant();
			}

			Dictionary old_dict = p_value;
			Dictionary new_dict;

			const Array &keys = old_dict.keys();
			for (int i = 0; i < keys.size(); i++) {
				const Variant &key = keys[i];
				new_dict[key.stringify()] = sanitize_variant(old_dict[key], p_depth + 1);
			}

			return new_dict;
		} break;
		case Variant::ARRAY:
		case Variant::PACKED_BYTE_ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
		case Variant::PACKED_STRING_ARRAY:
		case Variant::PACKED_VECTOR2_ARRAY:
		case Variant::PACKED_VECTOR3_ARRAY:
		case Variant::PACKED_COLOR_ARRAY:
		case Variant::PACKED_VECTOR4_ARRAY: {
			if (p_depth > VARIANT_CONVERSION_MAX_DEPTH) {
				ERR_PRINT_ONCE("Sentry: Maximum Variant depth reached!");
				return Variant();
			}

			Array arr;
			bool oob = false;
			bool valid = true;
			int i = 0;

			do {
				Variant item = p_value.get_indexed(i++, valid, oob);
				if (valid) {
					arr.append(sanitize_variant(item, p_depth + 1));
				}
			} while (!oob);

			return arr;
		} break;
		case Variant::VECTOR2:
		case Variant::VECTOR2I:
		case Variant::RECT2:
		case Variant::RECT2I:
		case Variant::VECTOR3:
		case Variant::VECTOR3I:
		case Variant::TRANSFORM2D:
		case Variant::VECTOR4:
		case Variant::VECTOR4I:
		case Variant::PLANE:
		case Variant::QUATERNION:
		case Variant::AABB:
		case Variant::BASIS:
		case Variant::TRANSFORM3D:
		case Variant::PROJECTION:
		case Variant::COLOR:
		case Variant::STRING_NAME:
		case Variant::NODE_PATH:
		case Variant::RID:
		case Variant::OBJECT:
		case Variant::CALLABLE:
		case Variant::SIGNAL: {
			return p_value.stringify();
		} break;
		default: {
			return p_value;
		} break;
	}
}

} //namespace sentry::android
