#ifndef COCOA_UTIL_H
#define COCOA_UTIL_H

#import <Foundation/Foundation.h>
#import <Sentry/Sentry.h>

enum SentryLevel:NSUInteger {
  kSentryLevelNone  = 0,
  kSentryLevelDebug = 1,
  kSentryLevelInfo = 2,
  kSentryLevelWarning = 3,
  kSentryLevelError = 4,
  kSentryLevelFatal = 5,
};

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "sentry/level.h"

using namespace godot;

namespace sentry::cocoa {

SentryLevel levelToSentryLevel(sentry::Level p_level);
sentry::Level sentryLevelToLevel(SentryLevel p_sentry_level);
NSString* stringToNSString(const String& s);
NSDictionary* dictTONSDict(const Dictionary &p_value);
NSMutableArray *packedStringArrayToNSArray(PackedStringArray arr);
NSMutableArray *arrayToNSArray(Array arr);

}

#endif // COCOA_UTIL_H
