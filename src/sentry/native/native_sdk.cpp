#include "native_sdk.h"

#include "../../sentry_options.h"
#include "../../sentry_sdk.h"
#include "../../sentry_util.h"
#include "../environment.h"

#include <sentry.h>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/string.hpp>

namespace sentry {

void NativeSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(p_key.is_empty());
	sentry_set_context(p_key.utf8(), SentryUtil::variant_to_sentry_value(p_value));
}

void NativeSDK::remove_context(const String &p_key) {
	ERR_FAIL_COND(p_key.is_empty());
	sentry_remove_context(p_key.utf8());
}

void NativeSDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(p_key.is_empty());
	sentry_set_tag(p_key.utf8(), p_value.utf8());
}

void NativeSDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND(p_key.is_empty());
	sentry_remove_tag(p_key.utf8());
}

void NativeSDK::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_NULL(p_user);

	sentry_value_t user_data = sentry_value_new_object();

	if (!p_user->get_id().is_empty()) {
		sentry_value_set_by_key(user_data, "id",
				sentry_value_new_string(p_user->get_id().utf8()));
	}
	if (!p_user->get_username().is_empty()) {
		sentry_value_set_by_key(user_data, "username",
				sentry_value_new_string(p_user->get_username().utf8()));
	}
	if (!p_user->get_email().is_empty()) {
		sentry_value_set_by_key(user_data, "email",
				sentry_value_new_string(p_user->get_email().utf8()));
	}
	if (!p_user->get_ip_address().is_empty()) {
		sentry_value_set_by_key(user_data, "ip_address",
				sentry_value_new_string(p_user->get_ip_address().utf8()));
	}
	sentry_set_user(user_data);
}

void NativeSDK::remove_user() {
	sentry_remove_user();
}

void NativeSDK::add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
		const String &p_type, const Dictionary &p_data) {
	sentry_value_t crumb = sentry_value_new_breadcrumb(p_type.utf8().ptr(), p_message.utf8().ptr());
	sentry_value_set_by_key(crumb, "category", sentry_value_new_string(p_category.utf8().ptr()));
	sentry_value_set_by_key(crumb, "level", sentry_value_new_string(sentry::level_as_cstring(p_level)));
	sentry_value_set_by_key(crumb, "data", SentryUtil::variant_to_sentry_value(p_data));
	sentry_add_breadcrumb(crumb);
}

void NativeSDK::capture_message(const String &p_message, Level p_level, const String &p_logger) {
	sentry_value_t event = sentry_value_new_message_event(
			(sentry_level_t)p_level,
			p_logger.utf8().get_data(),
			p_message.utf8().get_data());
	last_uuid = sentry_capture_event(event);
}

String NativeSDK::get_last_event_id() {
	char str[37];
	sentry_uuid_as_string(&last_uuid, str);
	return str;
}

void NativeSDK::initialize() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	sentry_options_t *options = sentry_options_new();
	sentry_options_set_dsn(options, SentryOptions::get_singleton()->get_dsn());

	// Establish handler path.
	String handler_fn;
#ifdef LINUX_ENABLED
	handler_fn = "crashpad_handler";
#elif WINDOWS_ENABLED
	handler_fn = "crashpad_handler.exe";
#elif MACOS_ENABLED
	handler_fn = "crashpad_handler";
#endif
	String handler_path = OS::get_singleton()->get_executable_path().get_base_dir() + "/" + handler_fn;
	if (!FileAccess::file_exists(handler_path)) {
		handler_path = ProjectSettings::get_singleton()->globalize_path("res://addons/sentrysdk/bin/" + handler_fn);
	}
	if (FileAccess::file_exists(handler_path)) {
		sentry_options_set_handler_path(options, handler_path.utf8());
	} else {
		ERR_PRINT(vformat("Sentry: Failed to locate crash handler (crashpad) - backend disabled (%s)", handler_path));
		sentry_options_set_backend(options, NULL);
	}

	sentry_options_set_database_path(options, (OS::get_singleton()->get_user_data_dir() + "/sentry").utf8());
	sentry_options_set_sample_rate(options, SentryOptions::get_singleton()->get_sample_rate());
	sentry_options_set_release(options, SentryOptions::get_singleton()->get_release());
	sentry_options_set_debug(options, SentryOptions::get_singleton()->is_debug_enabled());
	sentry_options_set_environment(options, sentry::environment::get_environment());
	sentry_options_set_sdk_name(options, "sentry.native.godot");
	sentry_options_set_max_breadcrumbs(options, SentryOptions::get_singleton()->get_max_breadcrumbs());

	// Attach LOG file.
	// TODO: Decide whether log-file must be trimmed before send.
	if (SentryOptions::get_singleton()->is_attach_log_enabled()) {
		String log_path = ProjectSettings::get_singleton()->get_setting("debug/file_logging/log_path");
		if (FileAccess::file_exists(log_path)) {
			log_path = log_path.replace("user://", OS::get_singleton()->get_user_data_dir() + "/");
			sentry_options_add_attachment(options, log_path.utf8());
		} else {
			WARN_PRINT("Sentry: Log file not found. Make sure \"debug/file_logging/enable_file_logging\" is turned ON in the Project Settings.");
		}
	}

	// TODO: Fix hooks!
	// "before_send" hook.
	// auto before_send_handler = [](sentry_value_t event, void *hint, void *closure) {
	// 	return SentrySDK::get_singleton()->handle_before_send(event);
	// };
	// sentry_options_set_before_send(options, before_send_handler, NULL);

	// "on_crash" hook.
	// auto on_crash_handler = [](const sentry_ucontext_t *uctx, sentry_value_t event, void *closure) {
	// 	return SentrySDK::get_singleton()->handle_on_crash(event);
	// };
	// sentry_options_set_on_crash(options, on_crash_handler, NULL);

	sentry_init(options);
}

NativeSDK::~NativeSDK() {
	sentry_close();
}

} //namespace sentry