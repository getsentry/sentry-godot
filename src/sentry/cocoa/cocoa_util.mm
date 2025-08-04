#include "cocoa_util.h"

#include "sentry/common_defs.h"

using namespace godot;

namespace sentry::cocoa {

NSObject *variant_to_objc(const godot::Variant &p_value, int p_depth) {
	switch (p_value.get_type()) {
		case Variant::NIL: {
			return [NSNull null];
		}
		case Variant::BOOL: {
			return [NSNumber numberWithBool:(bool)p_value];
		}
		case Variant::INT: {
			return [NSNumber numberWithLong:(int64_t)p_value];
		}
		case Variant::FLOAT: {
			return [NSNumber numberWithDouble:(double)p_value];
		}
		case Variant::DICTIONARY: {
			if (p_depth > VARIANT_CONVERSION_MAX_DEPTH) {
				ERR_PRINT_ONCE("Sentry: Maximum Variant conversion depth reached!");
				return [NSString stringWithUTF8String:"{...}"];
			}

			Dictionary dict = p_value;
			NSMutableDictionary *objc_dict = [[NSMutableDictionary alloc] init];

			const Array &keys = dict.keys();
			for (int i = 0; i < keys.size(); i++) {
				const String &key = keys[i];
				const NSString *objc_key = [NSString stringWithUTF8String:key.utf8()];
				const NSObject *objc_value = variant_to_objc(dict[key], p_depth + 1);
				[objc_dict setObject:objc_value forKey:objc_key];
			}

			return objc_dict;
		}
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
				ERR_PRINT_ONCE("Sentry: Maximum Variant conversion depth reached!");
				return [NSString stringWithUTF8String:"[...]"];
			}

			NSMutableArray *objc_array = [[NSMutableArray alloc] init];
			bool oob = false;
			bool valid = true;
			int i = 0;

			do {
				Variant item = p_value.get_indexed(i++, valid, oob);
				if (valid) {
					[objc_array addObject:variant_to_objc(item, p_depth + 1)];
				}
			} while (!oob);

			return objc_array;
		}
		default: {
			return [NSString stringWithUTF8String:String(p_value).utf8()];
		}
	}
}

NSDictionary *dictionary_to_objc(const godot::Dictionary &p_dictionary) {
	return (NSDictionary *)variant_to_objc(p_dictionary);
}

} // namespace sentry::cocoa
