#include "cocoa_sdk.h"

#include "sentry/common_defs.h"
#include "sentry/processing/process_event.h"
#include "sentry/sentry_attachment.h"
#include "sentry/util/print.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include <Sentry/Sentry.h>

using namespace godot;

namespace objc {

using SentryOptions = SentryOptions;
using SentrySDK = SentrySDK;
using SentryEvent = SentryEvent;
using SentryUser = SentryUser;
using SentryLevel = SentryLevel;
using SentryBreadcrumb = SentryBreadcrumb;
using SentryId = SentryId;
using SentryScope = SentryScope;

} // namespace objc

namespace {

objc::SentryLevel sentry_level_to_objc(sentry::Level p_level) {
	switch (p_level) {
		case sentry::Level::LEVEL_DEBUG:
			return objc::SentryLevel(1);
		case sentry::Level::LEVEL_INFO:
			return objc::SentryLevel(2);
		case sentry::Level::LEVEL_WARNING:
			return objc::SentryLevel(3);
		case sentry::Level::LEVEL_ERROR:
			return objc::SentryLevel(4);
		case sentry::Level::LEVEL_FATAL:
			return objc::SentryLevel(5);
		default:
			return objc::SentryLevel(4);
	}
}

} // unnamed namespace

namespace sentry {

void CocoaSDK::set_context(const String &p_key, const Dictionary &p_value) {
	[objc::SentrySDK configureScope:^(objc::SentryScope *scope) {
		NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];

		Array keys = p_value.keys();
		for (int i = 0; i < keys.size(); i++) {
			Variant k = keys[i];
			Variant v = p_value[k];

			NSString *keyStr = [NSString stringWithUTF8String:k.stringify().utf8()];
			// TODO: value has to be properly typed!
			NSString *valueStr = [NSString stringWithUTF8String:v.stringify().utf8()];

			[dict setObject:valueStr forKey:keyStr];
		}

		[scope setContextValue:dict forKey:[NSString stringWithUTF8String:p_key.utf8()]];
	}];
}

void CocoaSDK::remove_context(const String &p_key) {
	[objc::SentrySDK configureScope:^(objc::SentryScope *scope) {
		[scope removeContextForKey:[NSString stringWithUTF8String:p_key.utf8()]];
	}];
}

void CocoaSDK::set_tag(const String &p_key, const String &p_value) {
	[objc::SentrySDK configureScope:^(objc::SentryScope *scope) {
		[scope setTagValue:[NSString stringWithUTF8String:p_value.utf8()]
					forKey:[NSString stringWithUTF8String:p_key.utf8()]];
	}];
}

void CocoaSDK::remove_tag(const String &p_key) {
	[objc::SentrySDK configureScope:^(objc::SentryScope *scope) {
		[scope removeTagForKey:[NSString stringWithUTF8String:p_key.utf8()]];
	}];
}

void CocoaSDK::set_user(const Ref<SentryUser> &p_user) {
	if (p_user.is_valid()) {
		objc::SentryUser *user = [[objc::SentryUser alloc] init];

		if (!p_user->get_id().is_empty()) {
			user.userId = [NSString stringWithUTF8String:p_user->get_id().utf8()];
		}
		if (!p_user->get_username().is_empty()) {
			user.username = [NSString stringWithUTF8String:p_user->get_username().utf8()];
		}
		if (!p_user->get_email().is_empty()) {
			user.email = [NSString stringWithUTF8String:p_user->get_email().utf8()];
		}
		if (!p_user->get_ip_address().is_empty()) {
			user.ipAddress = [NSString stringWithUTF8String:p_user->get_ip_address().utf8()];
		}

		[objc::SentrySDK setUser:user];
	} else {
		remove_user();
	}
}

void CocoaSDK::remove_user() {
	[objc::SentrySDK setUser:nil];
}

void CocoaSDK::add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
		const String &p_type, const Dictionary &p_data) {
	objc::SentryBreadcrumb *breadcrumb = [[objc::SentryBreadcrumb alloc] init];
	breadcrumb.message = [NSString stringWithUTF8String:p_message.utf8()];
	breadcrumb.category = [NSString stringWithUTF8String:p_category.utf8()];
	breadcrumb.type = [NSString stringWithUTF8String:p_type.utf8()];
	breadcrumb.level = sentry_level_to_objc(p_level);

	[objc::SentrySDK addBreadcrumb:breadcrumb];
}

String CocoaSDK::capture_message(const String &p_message, Level p_level) {
	objc::SentryId *event_id = [objc::SentrySDK captureMessage:[NSString stringWithUTF8String:p_message.utf8()]
												withScopeBlock:^(objc::SentryScope *scope) {
													scope.level = sentry_level_to_objc(p_level);
												}];

	if (event_id) {
		return String([[event_id description] UTF8String]);
	}
	return "";
}

String CocoaSDK::get_last_event_id() {
	// TODO: figure out what to do with this (maybe handle last id via callbacks).
	return String();
}

Ref<SentryEvent> CocoaSDK::create_event() {
	// TODO: Implement
	return nullptr;
}

String CocoaSDK::capture_event(const Ref<SentryEvent> &p_event) {
	// TODO: Implement
	return "";
}

void CocoaSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	// TODO: Implement
}

void CocoaSDK::initialize(const PackedStringArray &p_global_attachments) {
	String dsn = SentryOptions::get_singleton()->get_dsn();
	bool debug_enabled = SentryOptions::get_singleton()->is_debug_enabled();

	[objc::SentrySDK startWithConfigureOptions:^(objc::SentryOptions *options) {
		options.dsn = [NSString stringWithUTF8String:dsn.utf8()];
		options.debug = debug_enabled;
	}];

	initialized = true;
}

CocoaSDK::~CocoaSDK() {
	if (initialized) {
		[objc::SentrySDK close];
	}
}

} // namespace sentry
