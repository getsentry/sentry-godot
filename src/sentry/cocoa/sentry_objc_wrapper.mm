#include "sentry_objc_wrapper.h"

#include <Foundation/Foundation.h>
#include <Sentry/Sentry.h>

using namespace godot;

namespace sentry::objc_wrapper {

void initialize_sentry_sdk(const String &dsn, bool debug_enabled) {
	[SentrySDK startWithConfigureOptions:^(SentryOptions *options) {
		options.dsn = [NSString stringWithUTF8String:dsn.utf8()];
		options.debug = debug_enabled;
	}];
}

void shutdown_sentry_sdk() {
	[SentrySDK close];
}

void set_user_objc(const String &user_id, const String &username, const String &email, const String &ip_address) {
	SentryUser *user = [[SentryUser alloc] init];

	if (!user_id.is_empty()) {
		user.userId = [NSString stringWithUTF8String:user_id.utf8()];
	}
	if (!username.is_empty()) {
		user.username = [NSString stringWithUTF8String:username.utf8()];
	}
	if (!email.is_empty()) {
		user.email = [NSString stringWithUTF8String:email.utf8()];
	}
	if (!ip_address.is_empty()) {
		user.ipAddress = [NSString stringWithUTF8String:ip_address.utf8()];
	}

	[SentrySDK setUser:user];
}

void remove_user_objc() {
	[SentrySDK setUser:nil];
}

void add_breadcrumb_objc(const String &message, const String &category, Level level, const String &type) {
	SentryBreadcrumb *breadcrumb = [[SentryBreadcrumb alloc] init];
	breadcrumb.message = [NSString stringWithUTF8String:message.utf8()];
	breadcrumb.category = [NSString stringWithUTF8String:category.utf8()];
	breadcrumb.type = [NSString stringWithUTF8String:type.utf8()];

	// TODO: figure out levels
	switch (level) {
		case Level::LEVEL_DEBUG:
			breadcrumb.level = (SentryLevel)1;
			break;
		case Level::LEVEL_INFO:
			breadcrumb.level = (SentryLevel)2;
			break;
		case Level::LEVEL_WARNING:
			breadcrumb.level = (SentryLevel)3;
			break;
		case Level::LEVEL_ERROR:
			breadcrumb.level = (SentryLevel)4;
			break;
		case Level::LEVEL_FATAL:
			breadcrumb.level = (SentryLevel)5;
			break;
	}

	[SentrySDK addBreadcrumb:breadcrumb];
}

String capture_message_objc(const String &message, Level level) {
	// TODO: figure out levels
	SentryLevel sentryLevel = (SentryLevel)3;
	switch (level) {
		case Level::LEVEL_DEBUG:
			sentryLevel = (SentryLevel)1;
			break;
		case Level::LEVEL_INFO:
			sentryLevel = (SentryLevel)2;
			break;
		case Level::LEVEL_WARNING:
			sentryLevel = (SentryLevel)3;
			break;
		case Level::LEVEL_ERROR:
			sentryLevel = (SentryLevel)4;
			break;
		case Level::LEVEL_FATAL:
			sentryLevel = (SentryLevel)5;
			break;
	}

	SentryId *eventId = [SentrySDK captureMessage:[NSString stringWithUTF8String:message.utf8()]
								   withScopeBlock:^(SentryScope *scope) {
									   scope.level = sentryLevel;
								   }];

	if (eventId) {
		// Use string representation of the event ID
		return String([[eventId description] UTF8String]);
	}
	return "";
}

String get_last_event_id_objc() {
	// Note: Sentry Cocoa SDK doesn't provide a getLastEventId method
	// Event IDs are returned directly from capture methods
	// TODO: figure out what to do with this (maybe handle last id via callbacks).
	return "";
}

void set_context_objc(const String &key, const Dictionary &value) {
	[SentrySDK configureScope:^(SentryScope *scope) {
		NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];

		Array keys = value.keys();
		for (int i = 0; i < keys.size(); i++) {
			Variant k = keys[i];
			Variant v = value[k];

			NSString *keyStr = [NSString stringWithUTF8String:String(k).utf8()];
			// TODO: value has to be properly typed!
			NSString *valueStr = [NSString stringWithUTF8String:String(v).utf8()];

			[dict setObject:valueStr forKey:keyStr];
		}

		[scope setContextValue:dict forKey:[NSString stringWithUTF8String:key.utf8()]];
	}];
}

void remove_context_objc(const String &key) {
	[SentrySDK configureScope:^(SentryScope *scope) {
		[scope removeContextForKey:[NSString stringWithUTF8String:key.utf8()]];
	}];
}

void set_tag_objc(const String &key, const String &value) {
	[SentrySDK configureScope:^(SentryScope *scope) {
		[scope setTagValue:[NSString stringWithUTF8String:value.utf8()]
					forKey:[NSString stringWithUTF8String:key.utf8()]];
	}];
}

void remove_tag_objc(const String &key) {
	[SentrySDK configureScope:^(SentryScope *scope) {
		[scope removeTagForKey:[NSString stringWithUTF8String:key.utf8()]];
	}];
}

} // namespace sentry::objc_wrapper
