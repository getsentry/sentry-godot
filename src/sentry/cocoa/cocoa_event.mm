#include "cocoa_event.h"

#include "cocoa_includes.h"
#include "cocoa_util.h"

namespace {

inline objc::SentryEvent *_get_typed_cocoa_event(const sentry::cocoa::CocoaEvent *p_event) {
	return (objc::SentryEvent *)p_event->get_cocoa_event();
}

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

} // namespace sentry::cocoa
