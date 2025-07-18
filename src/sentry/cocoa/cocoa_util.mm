#include "cocoa_util.h"

using namespace godot;

namespace sentry::cocoa {

NSObject *variant_to_objc(const godot::Variant &p_value) {
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
		case Variant::ARRAY: {
			Array arr = p_value;
			NSMutableArray *objc_array = [[NSMutableArray alloc] init];

			for (int i = 0; i < arr.size(); i++) {
				const NSObject *objc_value = variant_to_objc(arr[i]);
				[objc_array addObject:objc_value];
			}

			return objc_array;
		}
		case Variant::DICTIONARY: {
			Dictionary dict = p_value;
			NSMutableDictionary *objc_dict = [[NSMutableDictionary alloc] init];

			const Array &keys = dict.keys();
			for (int i = 0; i < keys.size(); i++) {
				const String &key = keys[i];
				const NSString *objc_key = [NSString stringWithUTF8String:key.utf8()];
				const NSObject *objc_value = variant_to_objc(dict[key]);
				[objc_dict setObject:objc_value forKey:objc_key];
			}

			return objc_dict;
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
