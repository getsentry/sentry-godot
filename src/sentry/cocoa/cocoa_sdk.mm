#include "cocoa_sdk.h"

#include "cocoa_event.h"
#include "cocoa_includes.h"
#include "cocoa_util.h"
#include "sentry/common_defs.h"
#include "sentry/processing/process_event.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_options.h"
#include "sentry/util/print.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#import <Sentry/PrivateSentrySDKOnly.h>

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

	return event_id ? string_from_objc([event_id sentryIdString]) : String();
}

String CocoaSDK::get_last_event_id() {
	std::lock_guard lock(last_event_id_mutex);
	return last_event_id;
}

Ref<SentryEvent> CocoaSDK::create_event() {
	objc::SentryEvent *cocoa_event = [[objc::SentryEvent alloc] init];
	return memnew(CocoaEvent(cocoa_event));
}

String CocoaSDK::capture_event(const Ref<SentryEvent> &p_event) {
	ERR_FAIL_COND_V_MSG(p_event.is_null(), String(), "Sentry: Can't capture event - event object is null.");
	CocoaEvent *typed_event = Object::cast_to<CocoaEvent>(p_event.ptr());
	objc::SentryEvent *cocoa_event = (objc::SentryEvent *)typed_event->get_cocoa_event();
	objc::SentryId *event_id = [objc::SentrySDK captureEvent:cocoa_event];
	return event_id ? string_from_objc([event_id sentryIdString]) : String();
}

void CocoaSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	ERR_FAIL_COND_MSG(p_attachment.is_null(), "Sentry: Can't add null attachment.");

	objc::SentryAttachment *attachment_objc = nil;

	if (!p_attachment->get_path().is_empty()) {
		ERR_FAIL_NULL(ProjectSettings::get_singleton());
		String absolute_path = ProjectSettings::get_singleton()->globalize_path(p_attachment->get_path());

		sentry::util::print_debug(vformat("attaching file: %s", absolute_path));

		String filename = p_attachment->get_filename().is_empty() ? p_attachment->get_path().get_file() : p_attachment->get_filename();
		attachment_objc = [[objc::SentryAttachment alloc] initWithPath:string_to_objc(p_attachment->get_path())
															  filename:string_to_objc(filename)
														   contentType:string_to_objc(p_attachment->get_content_type_or_default())];
	} else {
		PackedByteArray bytes = p_attachment->get_bytes();
		ERR_FAIL_COND_MSG(bytes.is_empty(), "Sentry: Can't add attachment with empty bytes and no file path.");
		NSData *bytes_objc = [NSData dataWithBytes:bytes.ptr() length:bytes.size()];

		sentry::util::print_debug(vformat("attaching bytes with filename: %s", p_attachment->get_filename()));

		attachment_objc = [[objc::SentryAttachment alloc] initWithData:bytes_objc
															  filename:string_to_objc(p_attachment->get_filename())
														   contentType:string_to_objc(p_attachment->get_content_type_or_default())];
	}

	ERR_FAIL_NULL_MSG(attachment_objc, "Sentry: Failed to create Cocoa attachment object from the provided SentryAttachment data.");

	[objc::SentrySDK configureScope:^(objc::SentryScope *scope) {
		[scope addAttachment:attachment_objc];
	}];
}

void CocoaSDK::initialize(const PackedStringArray &p_global_attachments) {
	[objc::SentrySDK startWithConfigureOptions:^(objc::SentryOptions *options) {
		options.dsn = string_to_objc(SentryOptions::get_singleton()->get_dsn());
		options.debug = SentryOptions::get_singleton()->is_debug_enabled();
		options.releaseName = string_to_objc(SentryOptions::get_singleton()->get_release());
		options.environment = string_to_objc(SentryOptions::get_singleton()->get_environment());
		options.sampleRate = double_to_objc(SentryOptions::get_singleton()->get_sample_rate());
		options.maxBreadcrumbs = (NSUInteger)SentryOptions::get_singleton()->get_max_breadcrumbs();
		options.sendDefaultPii = SentryOptions::get_singleton()->is_send_default_pii_enabled();
		options.diagnosticLevel = sentry_level_to_objc(SentryOptions::get_singleton()->get_diagnostic_level());

		String dist = SentryOptions::get_singleton()->get_dist();
		if (!dist.is_empty()) {
			options.dist = string_to_objc(dist);
		}

		// NOTE: This only works for captureMessage(), unfortunately.
		options.attachStacktrace = false;

		[PrivateSentrySDKOnly setSdkName:@"sentry.cocoa.godot"];

		options.initialScope = ^(objc::SentryScope *scope) {
			// Add global attachments
			for (const String &path : p_global_attachments) {
				sentry::util::print_debug("adding attachment \"", path, "\"");
				objc::SentryAttachment *att = nil;
				if (path.ends_with(SENTRY_VIEW_HIERARCHY_FN)) {
					// TODO: Can't specify attachmentType!
					att = [[objc::SentryAttachment alloc] initWithPath:string_to_objc(path)
															  filename:string_to_objc("view-hierarchy.json")
														   contentType:string_to_objc("application/json")];
				} else {
					att = [[objc::SentryAttachment alloc] initWithPath:string_to_objc(path)];
				}
				ERR_CONTINUE(att == nil);
				[scope addAttachment:att];
			}
			return scope;
		};

		options.beforeSend = ^objc::SentryEvent *(objc::SentryEvent *event) {
			if (event.error == nil) {
				// HACK: Remove threads info for non-crash events.
				//       Threads override our custom-built stacktrace for scripts.
				// TODO: This needs to be disabled in sentry-cocoa.
				event.threads = nil;
				event.debugMeta = nil;
			}

			Ref<CocoaEvent> event_obj = memnew(CocoaEvent(event));
			Ref<CocoaEvent> processed = sentry::process_event(event_obj);

			if (unlikely(processed.is_null())) {
				return nil;
			} else {
				std::lock_guard lock(last_event_id_mutex);
				last_event_id = string_from_objc(event.eventId.sentryIdString);
				return event;
			}
		};
	}];
}

bool CocoaSDK::is_enabled() const {
	return [objc::SentrySDK isEnabled];
}

CocoaSDK::~CocoaSDK() {
	if (is_enabled()) {
		[objc::SentrySDK close];
	}
}

} //namespace sentry::cocoa
