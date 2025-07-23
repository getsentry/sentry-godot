#include "cocoa_event.h"

#include "cocoa_util.h"
#include "sentry/util/timestamp.h"

namespace {

inline NSMutableDictionary *as_mutable_dict(NSDictionary *p_dict) {
	if ([p_dict isKindOfClass:[NSMutableDictionary class]]) {
		return (NSMutableDictionary *)p_dict;
	} else {
		return [p_dict mutableCopy] ?: [NSMutableDictionary dictionary];
	}
}

// clang-format off
// Access NSDictionary property of an object as mutable (create a copy if needed).
#define AS_MUTABLE_DICT(obj, prop) 												\
    ([obj.prop isKindOfClass:[NSMutableDictionary class]] ? 					\
    (NSMutableDictionary *)obj.prop : 											\
    (NSMutableDictionary *)(obj.prop = ([obj.prop mutableCopy] ?: [NSMutableDictionary dictionary])))

// Access NSArray property of an object as mutable (create a copy if needed).
#define AS_MUTABLE_ARRAY(obj, prop)												\
	([obj.prop isKindOfClass:[NSMutableArray class]] ? 							\
	(NSMutableArray *)obj.prop : 												\
	(NSMutableArray *)(obj.prop = ([obj.prop mutableCopy] ?: [NSMutableArray array])))

// clang-format on

} // unnamed namespace

namespace sentry::cocoa {

String CocoaEvent::get_id() const {
	ERR_FAIL_NULL_V(cocoa_event, String());
	return string_from_objc(cocoa_event.eventId.sentryIdString);
}

void CocoaEvent::set_message(const String &p_message) {
	ERR_FAIL_NULL(cocoa_event);

	if (p_message.is_empty()) {
		cocoa_event.message = nil;
	} else {
		cocoa_event.message = [[objc::SentryMessage alloc] initWithFormatted:string_to_objc(p_message)];
	}
}

String CocoaEvent::get_message() const {
	ERR_FAIL_NULL_V(cocoa_event, String());

	if (cocoa_event.message) {
		return string_from_objc(cocoa_event.message.formatted);
	} else {
		return String();
	}
}

void CocoaEvent::set_timestamp(const String &p_timestamp) {
	ERR_FAIL_NULL(cocoa_event);

	if (p_timestamp.is_empty()) {
		cocoa_event.timestamp = nil;
		return;
	}

	int64_t microseconds = sentry::util::rfc3339_timestamp_to_microseconds(p_timestamp.ascii());
	NSTimeInterval seconds = microseconds * 0.000'001;

	cocoa_event.timestamp = [NSDate dateWithTimeIntervalSince1970:seconds];
}

String CocoaEvent::get_timestamp() const {
	ERR_FAIL_NULL_V(cocoa_event, String());

	if (cocoa_event.timestamp == nil) {
		return String();
	}

	NSTimeInterval seconds = [cocoa_event.timestamp timeIntervalSince1970];
	int64_t microseconds = (int64_t)(seconds * 1000'000.0);

	return sentry::util::microseconds_to_rfc3339_timestamp(microseconds);
}

String CocoaEvent::get_platform() const {
	ERR_FAIL_NULL_V(cocoa_event, String());

	return string_from_objc(cocoa_event.platform);
}

void CocoaEvent::set_level(sentry::Level p_level) {
	ERR_FAIL_NULL(cocoa_event);

	cocoa_event.level = sentry_level_to_objc(p_level);
}

sentry::Level CocoaEvent::get_level() const {
	ERR_FAIL_NULL_V(cocoa_event, sentry::Level::LEVEL_ERROR);

	return sentry_level_from_objc(cocoa_event.level);
}

void CocoaEvent::set_logger(const String &p_logger) {
	ERR_FAIL_NULL(cocoa_event);

	cocoa_event.logger = string_to_objc_or_nil_if_empty(p_logger);
}

String CocoaEvent::get_logger() const {
	ERR_FAIL_NULL_V(cocoa_event, String());

	return string_from_objc(cocoa_event.logger);
}

void CocoaEvent::set_release(const String &p_release) {
	ERR_FAIL_NULL(cocoa_event);

	cocoa_event.releaseName = string_to_objc_or_nil_if_empty(p_release);
}

String CocoaEvent::get_release() const {
	ERR_FAIL_NULL_V(cocoa_event, String());

	return string_from_objc(cocoa_event.releaseName);
}

void CocoaEvent::set_dist(const String &p_dist) {
	ERR_FAIL_NULL(cocoa_event);

	cocoa_event.dist = string_to_objc_or_nil_if_empty(p_dist);
}

String CocoaEvent::get_dist() const {
	ERR_FAIL_NULL_V(cocoa_event, String());

	return string_from_objc(cocoa_event.dist);
}

void CocoaEvent::set_environment(const String &p_environment) {
	ERR_FAIL_NULL(cocoa_event);

	cocoa_event.environment = string_to_objc_or_nil_if_empty(p_environment);
}

String CocoaEvent::get_environment() const {
	ERR_FAIL_NULL_V(cocoa_event, String());

	return string_from_objc(cocoa_event.environment);
}

void CocoaEvent::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_NULL(cocoa_event);

	NSString *k = string_to_objc(p_key);
	NSString *v = string_to_objc(p_value);

	NSMutableDictionary *mut_tags = AS_MUTABLE_DICT(cocoa_event, tags);
	mut_tags[k] = v;
}

void CocoaEvent::remove_tag(const String &p_key) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't remove tag with an empty key.");

	ERR_FAIL_NULL(cocoa_event);

	NSMutableDictionary *mut_tags = AS_MUTABLE_DICT(cocoa_event, tags);
	[mut_tags removeObjectForKey:string_to_objc(p_key)];
}

String CocoaEvent::get_tag(const String &p_key) {
	ERR_FAIL_COND_V_MSG(p_key.is_empty(), String(), "Sentry: Can't get tag with an empty key.");

	ERR_FAIL_NULL_V(cocoa_event, String());

	if (cocoa_event.tags) {
		NSString *v = [cocoa_event.tags objectForKey:string_to_objc(p_key)];
		return string_from_objc(v);
	}
	return String();
}

void CocoaEvent::merge_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't merge context with an empty key.");

	if (p_value.is_empty()) {
		return;
	}

	ERR_FAIL_NULL(cocoa_event);

	NSMutableDictionary *mut_contexts = AS_MUTABLE_DICT(cocoa_event, context);

	NSString *context_name = string_to_objc(p_key);

	NSDictionary *existing_context = [mut_contexts objectForKey:context_name];
	if (existing_context) {
		// If context exists, update it with new values.
		NSMutableDictionary *mut_exisiting_context = as_mutable_dict(existing_context);
		const Array &updated_keys = p_value.keys();
		for (int i = 0; i < updated_keys.size(); i++) {
			const String &key = updated_keys[i];
			mut_exisiting_context[string_to_objc(key)] = variant_to_objc(p_value[key]);
		}
		mut_contexts[context_name] = mut_exisiting_context;
	} else {
		// If context doesn't exist, just add it.
		mut_contexts[context_name] = dictionary_to_objc(p_value);
	}
}

void CocoaEvent::add_exception(const Exception &p_exception) {
	ERR_FAIL_NULL(cocoa_event);

	NSMutableArray *mut_frames = [NSMutableArray arrayWithCapacity:p_exception.frames.size()];
	for (const StackFrame &frame : p_exception.frames) {
		objc::SentryFrame *cocoa_frame = [[objc::SentryFrame alloc] init];
		cocoa_frame.fileName = string_to_objc(frame.filename);
		cocoa_frame.function = string_to_objc(frame.function);
		cocoa_frame.lineNumber = int_to_objc(frame.lineno);
		cocoa_frame.inApp = bool_to_objc(frame.in_app);
		cocoa_frame.platform = string_to_objc(frame.platform);

		// TODO: unable to pass context_line, pre_context, post_context.
		// TODO: unable to pass local/member vars.

		[mut_frames addObject:cocoa_frame];
	}

	objc::SentryStacktrace *stack_trace = [[objc::SentryStacktrace alloc] initWithFrames:mut_frames
																			   registers:[NSDictionary dictionary]];
	objc::SentryException *cocoa_exception = [[objc::SentryException alloc] initWithValue:string_to_objc(p_exception.value)
																					 type:string_to_objc(p_exception.type)];
	cocoa_exception.stacktrace = stack_trace;
	NSMutableArray *mut_exceptions = AS_MUTABLE_ARRAY(cocoa_event, exceptions);
	[mut_exceptions addObject:cocoa_exception];
}

bool CocoaEvent::is_crash() const {
	ERR_FAIL_NULL_V(cocoa_event, false);

	return cocoa_event.error != nil;
}

CocoaEvent::CocoaEvent() {
	objc::SentryEvent *ev = [[objc::SentryEvent alloc] init];
	cocoa_event = [ev retain];
}

CocoaEvent::CocoaEvent(objc::SentryEvent *p_cocoa_event) {
	cocoa_event = [(id)p_cocoa_event retain];
}

CocoaEvent::~CocoaEvent() {
	if (cocoa_event) {
		[(id)cocoa_event release];
		cocoa_event = nil;
	}
}

} // namespace sentry::cocoa
