#include "cocoa_event.h"

#include "cocoa_includes.h"
#include "cocoa_util.h"

namespace {

inline objc::SentryEvent *_get_typed_cocoa_event(const sentry::cocoa::CocoaEvent *p_event) {
	return (objc::SentryEvent *)p_event->get_cocoa_event();
}

inline NSMutableDictionary *as_mutable_dict(NSDictionary *p_dict) {
	if ([p_dict isKindOfClass:[NSMutableDictionary class]]) {
		return (NSMutableDictionary *)p_dict;
	} else {
		return [p_dict mutableCopy] ?: [NSMutableDictionary dictionary];
	}
}

// clang-format off
// Access NSDictionary property of an object as mutable (create mutable copy if needed).
#define AS_MUTABLE_DICT(obj, prop) 												\
    ([obj.prop isKindOfClass:[NSMutableDictionary class]] ? 					\
    (NSMutableDictionary *)obj.prop : 											\
    (NSMutableDictionary *)(obj.prop = ([obj.prop mutableCopy] ?: [NSMutableDictionary dictionary])))
// clang-format on

} // unnamed namespace

namespace sentry::cocoa {

String CocoaEvent::get_id() const {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL_V(cocoa_event, String());
	return string_from_objc(cocoa_event.eventId.sentryIdString);
}

void CocoaEvent::set_message(const String &p_message) {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL(cocoa_event);

	if (p_message.is_empty()) {
		cocoa_event.message = nil;
	} else {
		cocoa_event.message = [[objc::SentryMessage alloc] initWithFormatted:string_to_objc(p_message)];
	}
}

String CocoaEvent::get_message() const {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL_V(cocoa_event, String());

	if (cocoa_event.message) {
		return string_from_objc(cocoa_event.message.formatted);
	} else {
		return String();
	}
}

void CocoaEvent::set_timestamp(const String &p_timestamp) {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL(cocoa_event);

	ERR_FAIL_MSG("not implemented");
}

String CocoaEvent::get_timestamp() const {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL_V(cocoa_event, String());

	ERR_FAIL_V_MSG(String(), "not implemented");
}

String CocoaEvent::get_platform() const {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL_V(cocoa_event, String());

	return string_from_objc(cocoa_event.platform);
}

void CocoaEvent::set_level(sentry::Level p_level) {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL(cocoa_event);

	cocoa_event.level = sentry_level_to_objc(p_level);
}

sentry::Level CocoaEvent::get_level() const {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL_V(cocoa_event, sentry::Level::LEVEL_ERROR);

	return sentry_level_from_objc(cocoa_event.level);
}

void CocoaEvent::set_logger(const String &p_logger) {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL(cocoa_event);

	cocoa_event.logger = string_to_objc_or_nil_if_empty(p_logger);
}

String CocoaEvent::get_logger() const {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL_V(cocoa_event, String());

	return string_from_objc(cocoa_event.logger);
}

void CocoaEvent::set_release(const String &p_release) {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL(cocoa_event);

	cocoa_event.releaseName = string_to_objc_or_nil_if_empty(p_release);
}

String CocoaEvent::get_release() const {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL_V(cocoa_event, String());

	return string_from_objc(cocoa_event.releaseName);
}

void CocoaEvent::set_dist(const String &p_dist) {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL(cocoa_event);

	cocoa_event.dist = string_to_objc_or_nil_if_empty(p_dist);
}

String CocoaEvent::get_dist() const {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL_V(cocoa_event, String());

	return string_from_objc(cocoa_event.dist);
}

void CocoaEvent::set_environment(const String &p_environment) {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL(cocoa_event);

	cocoa_event.environment = string_to_objc_or_nil_if_empty(p_environment);
}

String CocoaEvent::get_environment() const {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL_V(cocoa_event, String());

	return string_from_objc(cocoa_event.environment);
}

void CocoaEvent::set_tag(const String &p_key, const String &p_value) {
	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL(cocoa_event);

	NSString *k = string_to_objc(p_key);
	NSString *v = string_to_objc(p_value);

	if ([cocoa_event.tags isKindOfClass:[NSMutableDictionary class]]) {
		// Mutable dictionary – avoid copy.
		((NSMutableDictionary *)cocoa_event.tags)[k] = v;
	} else {
		// Not mutable or nil.
		NSMutableDictionary *mutable_tags = cocoa_event.tags ? [cocoa_event.tags mutableCopy] : [[NSMutableDictionary alloc] init];
		mutable_tags[k] = v;
		cocoa_event.tags = mutable_tags;
	}
}

void CocoaEvent::remove_tag(const String &p_key) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't remove tag with an empty key.");

	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
	ERR_FAIL_NULL(cocoa_event);

	NSString *k = string_to_objc(p_key);

	if ([cocoa_event.tags isKindOfClass:[NSMutableDictionary class]]) {
		// Mutable – avoid copy.
		[((NSMutableDictionary *)cocoa_event.tags) removeObjectForKey:k];
	} else if (cocoa_event.tags) {
		// Not mutable and not nil.
		NSMutableDictionary *mutable_tags = [cocoa_event.tags mutableCopy];
		[mutable_tags removeObjectForKey:k];
		cocoa_event.tags = mutable_tags;
	}
}

String CocoaEvent::get_tag(const String &p_key) {
	ERR_FAIL_COND_V_MSG(p_key.is_empty(), String(), "Sentry: Can't get tag with an empty key.");

	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
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

	objc::SentryEvent *cocoa_event = _get_typed_cocoa_event(this);
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

} // namespace sentry::cocoa
