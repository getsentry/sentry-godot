#include "cocoa_util.h"
#include "cocoa_uuid.h"


#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/array.hpp>

namespace {

NSObject *variantToNSObject(Variant value) {
    NSObject *objectValue = nil;

    switch (value.get_type()) {
        case Variant::BOOL:
            objectValue = @((bool) value);
            break;
        case Variant::INT:
            objectValue = @((int) value);
            break;
        case Variant::FLOAT:
            objectValue = @((float) value);
            break;
        case Variant::STRING:
            objectValue = sentry::cocoa::stringToNSString((String) value);
            break;
        case Variant::DICTIONARY:
            objectValue = sentry::cocoa::dictTONSDict((Dictionary) value);
            break;
        case Variant::PACKED_STRING_ARRAY:
            objectValue = sentry::cocoa::packedStringArrayToNSArray((PackedStringArray) value);
            break;
        case Variant::ARRAY:
            objectValue = sentry::cocoa::arrayToNSArray((Array) value);
            break;
        case Variant::VECTOR2I:
            {
                Vector2i v = value;
                String s = vformat("(%d, %d)", v.x, v.y);
                objectValue = sentry::cocoa::stringToNSString(s);
            }
            break;
        // Add more cases as needed for other types.
        default:
            objectValue = nil;
            break;
    }
    return objectValue;
}

} // unnamed namespace


namespace sentry::cocoa {

String make_uuid() {
    NSUUID *uuid = [NSUUID UUID];
    NSString *uuidString = [uuid UUIDString];
    String godotUUID = String::utf8([uuidString UTF8String]);
    return godotUUID;
}

SentryLevel levelToSentryLevel(sentry::Level p_level) {
    switch (p_level) {
        case LEVEL_NONE:
            return kSentryLevelNone;
        case LEVEL_DEBUG:
            return kSentryLevelDebug;
        case LEVEL_INFO:
            return kSentryLevelInfo;
        case LEVEL_WARNING:
            return kSentryLevelWarning;
        case LEVEL_ERROR:
            return kSentryLevelError;
        case LEVEL_FATAL:
            return kSentryLevelFatal;
    }
    return kSentryLevelNone;
}
sentry::Level sentryLevelToLevel(SentryLevel p_sentry_level) {
    switch (p_sentry_level) {
        case kSentryLevelNone:
            return LEVEL_NONE;
        case kSentryLevelDebug:
            return LEVEL_DEBUG;
        case kSentryLevelInfo:
            return LEVEL_INFO;
        case kSentryLevelWarning:
            return LEVEL_WARNING;
        case kSentryLevelError:
            return LEVEL_ERROR;
        case kSentryLevelFatal:
            return LEVEL_FATAL;
    }
    return LEVEL_NONE;
}

NSString* stringToNSString(const String& s) {
	NSString *ns = [NSString stringWithUTF8String:s.utf8()];
	return ns;
}

NSDictionary* dictTONSDict(const Dictionary &p_value) {
    NSMutableDictionary *nsDictionary = [[NSMutableDictionary alloc] init];
    Array keys = p_value.keys();
    for (int i = 0; i < keys.size(); ++i) {
        Variant key = keys[i];
        Variant value = p_value[key];
        NSObject *objectValue = variantToNSObject(value);
		if (objectValue != nil) {
        	[nsDictionary setObject:objectValue forKey:stringToNSString((String) key)];
        } else {
            ERR_PRINT(vformat("Unsupported type for conversion. Key: %s, Value Type: %s", (String) key, Variant::get_type_name(value.get_type())));
        }
    }

    return nsDictionary;
}

NSMutableArray *packedStringArrayToNSArray(PackedStringArray arr) {
	NSMutableArray *res = [[NSMutableArray alloc] init];
    for (int i = 0; i < arr.size(); ++i) {
		NSString *s = stringToNSString(arr[i]);
		[res addObject:s];
	}
	return res;
}

NSMutableArray *arrayToNSArray(Array arr) {
    NSMutableArray *res = [[NSMutableArray alloc] init];
    for (int i = 0; i < arr.size(); ++i) {
        Variant val = arr[i];
        NSObject* objectValue = variantToNSObject(val);
        if (objectValue != nil) {
            [res addObject:objectValue];
        } else {
            ERR_PRINT(vformat("Unsupported type for conversion. Index: %d, Value Type: %s", i, Variant::get_type_name(val.get_type())));
        }
    }
    return res;
}


}
