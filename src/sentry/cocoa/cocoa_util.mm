#include "cocoa_util.h"

#include "sentry/common_defs.h"
#include "sentry/logging/print.h"

#include <cstring>

using namespace godot;

namespace sentry::cocoa {

NSObject *variant_to_scope_attribute(const Variant &p_value) {
	switch (p_value.get_type()) {
		case Variant::BOOL: {
			return [NSNumber numberWithBool:(bool)p_value];
		} break;
		case Variant::INT: {
			return [NSNumber numberWithLongLong:(int64_t)p_value];
		} break;
		case Variant::FLOAT: {
			return [NSNumber numberWithDouble:(double)p_value];
		} break;
		default: {
			return [NSString stringWithUTF8String:p_value.stringify().utf8()];
		} break;
	}
}

SentryObjCAttachmentType attachment_type_to_objc(const String &p_attachment_type) {
	if (p_attachment_type == "event.view_hierarchy") {
		return SentryObjCAttachmentTypeViewHierarchy;
	}
	return SentryObjCAttachmentTypeEventAttachment;
}

SentryObjCAttachment *attachment_to_objc(const Ref<SentryAttachment> &p_attachment) {
	ERR_FAIL_COND_V_MSG(p_attachment.is_null(), nil, "Sentry: Can't add null attachment.");

	SentryObjCAttachment *attachment_objc = nil;

	if (!p_attachment->get_path().is_empty()) {
		String absolute_path = p_attachment->get_globalized_path();

		sentry::logging::print_debug(vformat("attaching file: %s", absolute_path));

		attachment_objc = [[SentryObjCAttachment alloc] initWithPath:string_to_objc(absolute_path)
															filename:string_to_objc(p_attachment->get_effective_filename())
														 contentType:string_to_objc(p_attachment->get_content_type_or_default())
													  attachmentType:attachment_type_to_objc(p_attachment->get_attachment_type())];
	} else {
		ERR_FAIL_COND_V_MSG(p_attachment->get_filename().is_empty(), nil, "Sentry: Can't add bytes attachment without filename.");
		PackedByteArray bytes = p_attachment->get_bytes();
		NSData *bytes_objc = [NSData dataWithBytes:bytes.ptr() length:bytes.size()];

		sentry::logging::print_debug("attaching bytes with filename: ", p_attachment->get_filename());

		attachment_objc = [[SentryObjCAttachment alloc] initWithData:bytes_objc
															filename:string_to_objc(p_attachment->get_filename())
														 contentType:string_to_objc(p_attachment->get_content_type_or_default())
													  attachmentType:attachment_type_to_objc(p_attachment->get_attachment_type())];
	}

	ERR_FAIL_NULL_V_MSG(attachment_objc, nil, "Sentry: Failed to create Cocoa attachment object from the provided SentryAttachment data.");

	return attachment_objc;
}

SentryObjCUser *user_to_objc(const Ref<SentryUser> &p_user) {
	if (p_user.is_null()) {
		return nil;
	}
	SentryObjCUser *user = [[SentryObjCUser alloc] init];
	user.userId = string_to_objc_or_nil_if_empty(p_user->get_id());
	user.username = string_to_objc_or_nil_if_empty(p_user->get_username());
	user.email = string_to_objc_or_nil_if_empty(p_user->get_email());
	user.ipAddress = string_to_objc_or_nil_if_empty(p_user->get_ip_address());
	return user;
}

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
			if (dict.is_empty()) {
				return @{};
			}
			NSMutableDictionary *objc_dict = [[NSMutableDictionary alloc] init];

			const Array &keys = dict.keys();
			for (int i = 0; i < keys.size(); i++) {
				const Variant &key = keys[i];
				const NSString *objc_key = [NSString stringWithUTF8String:key.stringify().utf8()];
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

			bool oob = false;
			bool valid = true;
			int i = 0;
			Variant item = p_value.get_indexed(i++, valid, oob);
			if (oob) {
				return @[];
			}
			NSMutableArray *objc_array = [[NSMutableArray alloc] init];

			do {
				if (valid) {
					[objc_array addObject:variant_to_objc(item, p_depth + 1)];
				}
				item = p_value.get_indexed(i++, valid, oob);
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
	if (p_array.is_empty()) {
		return @[];
	}

	NSMutableArray<NSString *> *objc_array = [NSMutableArray arrayWithCapacity:p_array.size()];
	for (int i = 0; i < p_array.size(); i++) {
		[objc_array addObject:string_to_objc(p_array[i])];
	}
	return objc_array;
}

} // namespace sentry::cocoa
