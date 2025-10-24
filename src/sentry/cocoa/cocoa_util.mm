#include "cocoa_util.h"

#include "sentry/common_defs.h"

#include <cstring>

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
			return [NSNumber numberWithLongLong:(int64_t)p_value];
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

godot::Variant variant_from_objc(const NSObject *p_value) {
	if ([p_value isKindOfClass:[NSNull class]]) {
		return Variant();
	} else if ([p_value isKindOfClass:[NSNumber class]]) {
		NSNumber *num = (NSNumber *)p_value;
		const char *t = [num objCType];

		if (strcmp(t, @encode(bool)) == 0 || strcmp(t, @encode(char)) == 0) {
			return [num boolValue];
		} else if (strcmp(t, @encode(double)) == 0) {
			return [num doubleValue];
		} else if (strcmp(t, @encode(int64_t)) == 0 || strcmp(t, @encode(long long)) == 0) {
			return [num longLongValue];
		} else if (strcmp(t, @encode(int)) == 0) {
			return [num intValue];
		} else if (strcmp(t, @encode(float)) == 0) {
			return [num floatValue];
		} else {
			ERR_PRINT("Sentry: Failed to convert NSNumber to Variant. Returning null.");
			return Variant();
		}
	} else if ([p_value isKindOfClass:[NSString class]]) {
		NSString *str = (NSString *)p_value;
		return String::utf8(str.UTF8String);
	} else if ([p_value isKindOfClass:[NSDictionary class]]) {
		godot::Dictionary godot_dict;
		NSDictionary *objc_dict = (NSDictionary *)p_value;
		for (id key in objc_dict) {
			godot_dict[variant_from_objc(key)] = variant_from_objc(objc_dict[key]);
		}
		return godot_dict;
	} else if ([p_value isKindOfClass:[NSArray class]]) {
		godot::Array godot_array;
		NSArray *objc_array = (NSArray *)p_value;
		for (id element in objc_array) {
			godot_array.push_back(variant_from_objc(element));
		}
		return godot_array;
	}

	ERR_PRINT("Sentry: Failed to convert ObjectiveC value to Variant. Returning null.");
	return Variant();
}

NSDictionary *dictionary_to_objc(const godot::Dictionary &p_dictionary) {
	return (NSDictionary *)variant_to_objc(p_dictionary);
}

NSArray<NSString *> *string_array_to_objc(const godot::PackedStringArray &p_array) {
	NSMutableArray<NSString *> *objc_array = [NSMutableArray arrayWithCapacity:p_array.size()];
	for (int i = 0; i < p_array.size(); i++) {
		[objc_array addObject:string_to_objc(p_array[i])];
	}
	return objc_array;
}

} // namespace sentry::cocoa
