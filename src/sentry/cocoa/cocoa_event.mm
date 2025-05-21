
#import <Sentry/Sentry.h>

// Note: This is included here, because the original class is defined in Swift
// and Sentry-Swift.h is not included with the HybridSDK.
@interface SentryId : NSObject
/// Returns a 32 lowercase character hexadecimal string description of the @c SentryId, such as
/// “12c2d058d58442709aa2eca08bf20986”.
@property (nonatomic, readonly, copy) NSString * _Nonnull sentryIdString;
@end

typedef SentryEvent ObjCSentryEvent;

#include "cocoa_event.h"

#include "godot_cpp/core/error_macros.hpp"
#include "sentry/level.h"
#include "sentry/cocoa/cocoa_util.h"



using namespace sentry::cocoa;

namespace {

inline ObjCSentryEvent *to_event(void *p_event) {
    ObjCSentryEvent *ev = (__bridge ObjCSentryEvent *) p_event;
	return ev;
}

} // unnamed namespace

SentryEvent *xxx(void *p_event) {
    return to_event(p_event);
}

String sentry::CocoaEvent::get_id() const {
    ObjCSentryEvent* ev = to_event(cocoa_event);
	NSString *uuidString = ev.eventId.sentryIdString;
	
	return String::utf8([uuidString UTF8String]);
}

void sentry::CocoaEvent::set_message(const String &p_message) {
    ObjCSentryEvent* ev = to_event(cocoa_event);
	if (p_message.is_empty()) {
		ev.message = nil;
	} else {
		ev.message = [[SentryMessage alloc] initWithFormatted:stringToNSString(p_message)];
	}
}

String sentry::CocoaEvent::get_message() const {
	ObjCSentryEvent *ev = to_event(cocoa_event);
	if (ev.message == nil) {
		return String();
	}
	SentryMessage *msg = ev.message;
	if (!msg.formatted) {
		return String();
	}
	String s = String::utf8([msg.formatted UTF8String]);
	return s;
}

void sentry::CocoaEvent::set_timestamp(const String &p_timestamp) {
	ObjCSentryEvent *ev = to_event(cocoa_event);

    // Convert Godot String to C-style string
    NSString *ns = stringToNSString(p_timestamp);

    // Create an NSDateFormatter
    NSDateFormatter *dateFormatter = [NSDateFormatter new];
    dateFormatter.dateFormat = @"yyyy-MM-dd'T'HH:mm:ss"; // Set the date format according to your string

    // Parse the C-style string into an NSDate
    NSDate *nsDate = [dateFormatter dateFromString:ns];

	ev.timestamp = nsDate;
}

String sentry::CocoaEvent::get_timestamp() const {
	ObjCSentryEvent *ev = to_event(cocoa_event);
	if (!ev.timestamp) {
        return String();
	}

    // Create an NSDateFormatter
    NSDateFormatter *dateFormatter = [NSDateFormatter new];
    dateFormatter.dateFormat = @"yyyy-MM-dd'T'HH:mm:ss"; // Set the date format according to your string
	NSString *ds = [dateFormatter stringFromDate:ev.timestamp];
	String s = String::utf8([ds UTF8String]);
	return s;
}

String sentry::CocoaEvent::get_platform() const {
	ObjCSentryEvent *ev = to_event(cocoa_event);
	String s = String::utf8([ev.platform UTF8String]);
	return s;
}

void sentry::CocoaEvent::set_level(sentry::Level p_level) {
    ObjCSentryEvent *ev = to_event(cocoa_event);
    [ev setLevel:levelToSentryLevel(p_level)];
}

sentry::Level sentry::CocoaEvent::get_level() const {
    ObjCSentryEvent *ev = to_event(cocoa_event);
    return sentryLevelToLevel([ev level]);
}

void sentry::CocoaEvent::set_logger(const String &p_logger) {
    ObjCSentryEvent *ev = to_event(cocoa_event);
    [ev setLogger:stringToNSString(p_logger)];
}

String sentry::CocoaEvent::get_logger() const {
    ObjCSentryEvent *ev = to_event(cocoa_event);
    NSString *nsLogger = [ev logger];
    String res = String::utf8([nsLogger UTF8String]);
    return res;
}

void sentry::CocoaEvent::set_release(const String &p_release) {
    ObjCSentryEvent *ev = to_event(cocoa_event);
    [ev setReleaseName:stringToNSString(p_release)];
}

String sentry::CocoaEvent::get_release() const {
    ObjCSentryEvent *ev = to_event(cocoa_event);
    NSString *nsRelease = [ev releaseName];
    String res = String::utf8([nsRelease UTF8String]);
    return res;
}

void sentry::CocoaEvent::set_dist(const String &p_dist) {
    ObjCSentryEvent *ev = to_event(cocoa_event);
    [ev setDist:stringToNSString(p_dist)];
}

String sentry::CocoaEvent::get_dist() const {
    ObjCSentryEvent *ev = to_event(cocoa_event);
    NSString *nsDist = [ev dist];
    String res = String::utf8([nsDist UTF8String]);
    return res;
}

void sentry::CocoaEvent::set_environment(const String &p_environment) {
    ObjCSentryEvent *ev = to_event(cocoa_event);
    [ev setEnvironment:stringToNSString(p_environment)];
}

String sentry::CocoaEvent::get_environment() const {
    ObjCSentryEvent *ev = to_event(cocoa_event);
    NSString *nsEnv = [ev environment];
    String res = String::utf8([nsEnv UTF8String]);
    return res;
}

void sentry::CocoaEvent::set_tag(const String &p_key, const String &p_value) {
    ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set tag with an empty key.");
    ObjCSentryEvent *ev = to_event(cocoa_event);
    NSMutableDictionary *tags = [NSMutableDictionary dictionaryWithDictionary:[ev tags]];
    [tags setObject:stringToNSString(p_value) forKey:stringToNSString(p_key)];
    [ev setTags:tags];
}

void sentry::CocoaEvent::remove_tag(const String &p_key) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't remove tag with an empty key.");
    ObjCSentryEvent *ev = to_event(cocoa_event);
    NSMutableDictionary *tags = [NSMutableDictionary dictionaryWithDictionary:[ev tags]];
    [tags removeObjectForKey:stringToNSString(p_key)];
}

String sentry::CocoaEvent::get_tag(const String &p_key) {
    ERR_FAIL_COND_V_MSG(p_key.is_empty(), String(), "Sentry: Can't get tag with an empty key.");
    ObjCSentryEvent *ev = to_event(cocoa_event);
    NSDictionary<NSString *, NSString *>* tags = [ev tags];
    NSString* nsTag = [tags objectForKey:stringToNSString(p_key)];
    String res = String::utf8([nsTag UTF8String]);
    return res;
}

sentry::CocoaEvent::CocoaEvent(void *p_event) {
    ObjCSentryEvent *ev = (__bridge ObjCSentryEvent *) p_event;
	cocoa_event = (__bridge_retained void *) ev;
}

sentry::CocoaEvent::CocoaEvent() {
	cocoa_event = (__bridge_retained void *) [[ObjCSentryEvent alloc] init];
}

sentry::CocoaEvent::~CocoaEvent() {
    CFRelease(cocoa_event);
    cocoa_event = nil;
}

