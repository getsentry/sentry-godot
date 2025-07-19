#include "cocoa_sdk.h"

#include "cocoa_includes.h"
#include "cocoa_util.h"
#include "sentry/common_defs.h"
#include "sentry/processing/process_event.h"
#include "sentry/sentry_attachment.h"
#include "sentry/util/print.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

namespace sentry::cocoa {

void CocoaSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(p_key.is_empty());
	[objc::SentrySDK configureScope:^(objc::SentryScope *scope) {
		[scope setContextValue:dictionary_to_objc(p_value) forKey:string_to_objc(p_key)];
	}];
}

void CocoaSDK::remove_context(const String &p_key) {
	ERR_FAIL_COND(p_key.is_empty());
	[objc::SentrySDK configureScope:^(objc::SentryScope *scope) {
		[scope removeContextForKey:string_to_objc(p_key)];
	}];
}

void CocoaSDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(p_key.is_empty());
	[objc::SentrySDK configureScope:^(objc::SentryScope *scope) {
		[scope setTagValue:string_to_objc(p_value) forKey:string_to_objc(p_key)];
	}];
}

void CocoaSDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND(p_key.is_empty());
	[objc::SentrySDK configureScope:^(objc::SentryScope *scope) {
		[scope removeTagForKey:string_to_objc(p_key)];
	}];
}

void CocoaSDK::set_user(const Ref<SentryUser> &p_user) {
	if (p_user.is_valid()) {
		objc::SentryUser *user = [[objc::SentryUser alloc] init];

		if (!p_user->get_id().is_empty()) {
			user.userId = string_to_objc(p_user->get_id());
		}
		if (!p_user->get_username().is_empty()) {
			user.username = string_to_objc(p_user->get_username());
		}
		if (!p_user->get_email().is_empty()) {
			user.email = string_to_objc(p_user->get_email());
		}
		if (!p_user->get_ip_address().is_empty()) {
			user.ipAddress = string_to_objc(p_user->get_ip_address());
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

	breadcrumb.level = sentry_level_to_objc(p_level);
	if (!p_message.is_empty()) {
		breadcrumb.message = string_to_objc(p_message);
	}
	if (!p_category.is_empty()) {
		breadcrumb.category = string_to_objc(p_category);
	}
	if (!p_type.is_empty()) {
		breadcrumb.type = string_to_objc(p_type);
	}
	if (!p_data.is_empty()) {
		breadcrumb.data = dictionary_to_objc(p_data);
	}

	[objc::SentrySDK addBreadcrumb:breadcrumb];
}

String CocoaSDK::capture_message(const String &p_message, Level p_level) {
	objc::SentryId *event_id = [objc::SentrySDK captureMessage:string_to_objc(p_message)
												withScopeBlock:^(objc::SentryScope *scope) {
													scope.level = sentry_level_to_objc(p_level);
												}];

	return event_id ? string_from_objc([event_id description]) : String();
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
	[objc::SentrySDK startWithConfigureOptions:^(objc::SentryOptions *options) {
		options.dsn = string_to_objc(SentryOptions::get_singleton()->get_dsn());
		options.debug = SentryOptions::get_singleton()->is_debug_enabled();
		options.releaseName = string_to_objc(SentryOptions::get_singleton()->get_release());
		options.dist = string_to_objc(SentryOptions::get_singleton()->get_dist());
		options.environment = string_to_objc(SentryOptions::get_singleton()->get_environment());
		options.sampleRate = double_to_objc(SentryOptions::get_singleton()->get_sample_rate());
		options.maxBreadcrumbs = (NSUInteger)SentryOptions::get_singleton()->get_max_breadcrumbs();
		// TODO: how to set sdk name?
	}];

	initialized = true;
}

CocoaSDK::~CocoaSDK() {
	if (initialized) {
		[objc::SentrySDK close];
	}
}

} //namespace sentry::cocoa
