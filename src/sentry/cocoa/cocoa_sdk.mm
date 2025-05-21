
#import <Sentry/Sentry.h>

typedef SentryUser ObjCSentryUser;
typedef SentryEvent ObjCSentryEvent;

// Note: This is included here, because the original class is defined in Swift
// and Sentry-Swift.h is not included with the HybridSDK.
@interface SentryId : NSObject
/// Returns a 32 lowercase character hexadecimal string description of the @c SentryId, such as
/// “12c2d058d58442709aa2eca08bf20986”.
@property (nonatomic, readonly, copy) NSString * _Nonnull sentryIdString;
@end

#include "cocoa_sdk.h"
#include "cocoa_event.h"

#include "sentry/contexts.h"
#include "sentry/level.h"
#include "sentry/util/print.h"
#include "sentry/util/screenshot.h"
#include "sentry/view_hierarchy.h"
#include "sentry_options.h"

#include "cocoa_util.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#ifdef DEBUG_ENABLED
#include <godot_cpp/classes/time.hpp>
#endif

#define _SCREENSHOT_FN "screenshot.jpg"
#define _VIEW_HIERARCHY_FN "view-hierarchy.json"


@interface SentryGodotFrame: SentryFrame
    /**
     * Context line
     */
    @property (nonatomic, copy) NSString *_Nullable contextLine;

    /**
     * Pre-Context
     */
    @property (nonatomic, copy) NSMutableArray<NSString *> *_Nullable preContext;

    /**
     * Post-Context
     */
    @property (nonatomic, copy) NSMutableArray<NSString *> *_Nullable postContext;

    - (NSDictionary<NSString *, id> *)serialize;
@end

@implementation SentryGodotFrame

- (NSDictionary<NSString *, id> *)serialize
{
    NSMutableDictionary *serializedData = [NSMutableDictionary new];

    [serializedData setValue:self.fileName forKey:@"filename"];
    [serializedData setValue:self.function forKey:@"function"];
    [serializedData setValue:self.lineNumber forKey:@"lineno"];
    [serializedData setValue:self.contextLine forKey:@"context_line"];
    [serializedData setValue:self.preContext forKey:@"pre_context"];
    [serializedData setValue:self.postContext forKey:@"post_context"];

    return serializedData;
}

@end


using namespace sentry::cocoa;

namespace sentry {

void CocoaSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(p_key.is_empty());
	NSString *nsKey = stringToNSString(p_key);
	NSDictionary *nsVal = dictTONSDict(p_value);
	[SentrySDK configureScope:^(SentryScope * _Nonnull scope) {
    	[scope setContextValue:nsVal forKey:nsKey];
	}];
}

void CocoaSDK::remove_context(const String &p_key) {
	ERR_FAIL_COND(p_key.is_empty());
	NSString *nsKey = stringToNSString(p_key);
	[SentrySDK configureScope:^(SentryScope * _Nonnull scope) {
    	[scope removeContextForKey:nsKey];
	}];
}

void CocoaSDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(p_key.is_empty());
	NSString *nsKey = stringToNSString(p_key);
	NSString *nsValue = stringToNSString(p_value);
	[SentrySDK configureScope:^(SentryScope * _Nonnull scope) {
    	[scope setTagValue:nsValue forKey:nsKey];
	}];
}

void CocoaSDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND(p_key.is_empty());
	NSString *nsKey = stringToNSString(p_key);
	[SentrySDK configureScope:^(SentryScope * _Nonnull scope) {
    	[scope removeTagForKey:nsKey];
	}];
}

void CocoaSDK::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_NULL(p_user);

    ObjCSentryUser *user = [[ObjCSentryUser alloc] init];

	if (!p_user->get_id().is_empty()) {
		user.userId = stringToNSString(p_user->get_id());
	}
	if (!p_user->get_username().is_empty()) {
		user.username = stringToNSString(p_user->get_username());
	}
	if (!p_user->get_email().is_empty()) {
		user.email = stringToNSString(p_user->get_email());
	}
	if (!p_user->get_ip_address().is_empty()) {
		user.ipAddress = stringToNSString(p_user->get_ip_address());
	}
	[SentrySDK setUser:user];
}

void CocoaSDK::remove_user() {
	[SentrySDK setUser:nil];
}

void CocoaSDK::add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
		const String &p_type, const Dictionary &p_data) {
	SentryBreadcrumb *crumb = [[SentryBreadcrumb alloc] init];
	crumb.type = stringToNSString(p_type);
	crumb.level = levelToSentryLevel(p_level);
	crumb.category = stringToNSString(p_category);
	crumb.message = stringToNSString(p_message);
	crumb.data = dictTONSDict(p_data);
	[SentrySDK addBreadcrumb:crumb];	
}

String CocoaSDK::capture_message(const String &p_message, Level p_level) {
	NSString *nsMsg = stringToNSString(p_message);

	// Assuming you have a SentryId object, for example from capturing an event
	SentryId *eventId = [SentrySDK captureMessage:nsMsg];

	// Convert the SentryId to a string
	NSString *uuidString = eventId.sentryIdString;
	
	last_uuid = String::utf8([uuidString UTF8String]);
	return last_uuid;
}

String CocoaSDK::get_last_event_id() {
	return last_uuid;
}

String CocoaSDK::capture_error(const String &p_type, const String &p_value, Level p_level, const Vector<StackFrame> &p_frames) {
	// Create a new event
	ObjCSentryEvent *event = [[ObjCSentryEvent alloc] init];

	// Set the level
	event.level = levelToSentryLevel(p_level);

	// Create exception
    NSString *nsValue = stringToNSString(p_value);
    NSString *nsType = stringToNSString(p_type);
	SentryException *exception = [[SentryException alloc] initWithValue:nsValue type:nsType];

	// Create frames for stacktrace
	NSMutableArray<SentryGodotFrame *> *frames = [NSMutableArray array];
	for (const StackFrame &frame : p_frames) {
		SentryGodotFrame *sentryFrame = [[SentryGodotFrame alloc] init];
		sentryFrame.fileName = stringToNSString(frame.filename);
		sentryFrame.function = stringToNSString(frame.function);
		sentryFrame.lineNumber = @(frame.lineno);

		if (!frame.context_line.is_empty()) {
			sentryFrame.contextLine = stringToNSString(frame.context_line);
			
			// Convert pre_context and post_context
			sentryFrame.preContext = arrayToNSArray(frame.pre_context);
			sentryFrame.postContext = arrayToNSArray(frame.post_context);
			// Add to frames array
			[frames addObject:sentryFrame];
		}
	}

	// Create stacktrace with frames
	SentryStacktrace *stacktrace = [[SentryStacktrace alloc] initWithFrames:frames registers:@{}];
	exception.stacktrace = stacktrace;

	// Add exception to event
	event.exceptions = @[exception];

	// Capture the event
	SentryId *eventId = [SentrySDK captureEvent:event];

	// Convert the SentryId to a string
	NSString *uuidString = eventId.sentryIdString;
	
	last_uuid = String::utf8([uuidString UTF8String]);
	return last_uuid;
}

Ref<SentryEvent> CocoaSDK::create_event() {
    ObjCSentryEvent* ev = [[ObjCSentryEvent alloc] init];
	Ref<SentryEvent> event = memnew(CocoaEvent((__bridge void *)ev));
	return event;
}

String CocoaSDK::capture_event(const Ref<SentryEvent> &p_event) {
    Ref<CocoaEvent> native_event = p_event;
	ObjCSentryEvent *event = (__bridge ObjCSentryEvent *) native_event->get_native_value();
    
    // Capture the event
    SentryId *eventId = [SentrySDK captureEvent:event];

    // Convert the SentryId to a string
    NSString *uuidString = eventId.sentryIdString;
    
    last_uuid = String::utf8([uuidString UTF8String]);
    return last_uuid;
}

void CocoaSDK::initialize() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	if (!OS::get_singleton()->has_feature("libgodot")) {
		// TODO: Implement native initialization
		ERR_PRINT("Only LibGodot setup is supported for now");
	}

	// Attach LOG file.
	// TODO: Decide whether log-file must be trimmed before send.
	if (SentryOptions::get_singleton()->is_attach_log_enabled()) {
		String log_path = ProjectSettings::get_singleton()->get_setting("debug/file_logging/log_path");
		if (FileAccess::file_exists(log_path)) {
			log_path = log_path.replace("user://", OS::get_singleton()->get_user_data_dir() + "/");

			// Convert Godot String to NSString
			NSString *nsLogPath = stringToNSString(log_path);

			// Create an attachment from a file path
			SentryAttachment *fileAttachment = [[SentryAttachment alloc] initWithPath:nsLogPath];

			// Add to global scope
			[SentrySDK configureScope:^(SentryScope * _Nonnull scope) {
				[scope addAttachment:fileAttachment];
			}];
		} else {
			WARN_PRINT("Sentry: Log file not found. Make sure \"debug/file_logging/enable_file_logging\" is turned ON in the Project Settings.");
		}
	}

	// Attach screenshot.
	if (SentryOptions::get_singleton()->is_attach_screenshot_enabled()) {
		String path = OS::get_singleton()->get_user_data_dir().path_join(_SCREENSHOT_FN);
		DirAccess::remove_absolute(path);
		// Convert Godot String to NSString
		NSString *nsPath = stringToNSString(path);

		// Create an attachment from a file path
		SentryAttachment *fileAttachment = [[SentryAttachment alloc] initWithPath:nsPath];

		// Add to global scope
		[SentrySDK configureScope:^(SentryScope * _Nonnull scope) {
			[scope addAttachment:fileAttachment];
		}];
	}

	// Attach view hierarchy (aka scene tree info).
	if (SentryOptions::get_singleton()->is_attach_scene_tree_enabled()) {
		String path = OS::get_singleton()->get_user_data_dir().path_join(_VIEW_HIERARCHY_FN);
		DirAccess::remove_absolute(path);
		// Convert Godot String to NSString
		NSString *nsPath = stringToNSString(path);

		// Create an attachment from a file path
		SentryAttachment *fileAttachment = [[SentryAttachment alloc] initWithPath:nsPath];

		// Add to global scope
		[SentrySDK configureScope:^(SentryScope * _Nonnull scope) {
			[scope addAttachment:fileAttachment];
		}];
	}

	initialized = true; // (err == 0);
}

CocoaSDK::~CocoaSDK() {
	if (!OS::get_singleton()->has_feature("libgodot")) {
		// TODO: Implement native initialization
		ERR_PRINT("Only LibGodot setup is supported for now");
	}
}

} //namespace sentry
