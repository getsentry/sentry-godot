#include "native_sdk.h"

#include "godot_cpp/core/error_macros.hpp"
#include "sentry.h"
#include "sentry/contexts.h"
#include "sentry/environment.h"
#include "sentry/level.h"
#include "sentry/native/native_util.h"
#include "sentry_options.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace {

void sentry_event_set_context(sentry_value_t p_event, const char *p_context_name, const Dictionary &p_context) {
	ERR_FAIL_COND(sentry_value_get_type(p_event) != SENTRY_VALUE_TYPE_OBJECT);
	ERR_FAIL_COND(strlen(p_context_name) == 0);

	if (p_context.is_empty()) {
		return;
	}

	sentry_value_t contexts = sentry_value_get_by_key(p_event, "contexts");
	if (sentry_value_is_null(contexts)) {
		contexts = sentry_value_new_object();
		sentry_value_set_by_key(p_event, "contexts", contexts);
	}

	// Check if context exists and update or add it.
	sentry_value_t ctx = sentry_value_get_by_key(contexts, p_context_name);
	if (!sentry_value_is_null(ctx)) {
		// If context exists, update it with new values.
		const Array &updated_keys = p_context.keys();
		for (int i = 0; i < updated_keys.size(); i++) {
			const String &key = updated_keys[i];
			sentry_value_set_by_key(ctx, key.utf8(), sentry::native::variant_to_sentry_value(p_context[key]));
		}
	} else {
		// If context doesn't exist, add it.
		sentry_value_set_by_key(contexts, p_context_name, sentry::native::variant_to_sentry_value(p_context));
	}
}

inline void inject_contexts(sentry_value_t p_event) {
	ERR_FAIL_COND(sentry_value_get_type(p_event) != SENTRY_VALUE_TYPE_OBJECT);

	HashMap<String, Dictionary> contexts = sentry::contexts::make_event_contexts();
	for (const auto &kv : contexts) {
		sentry_event_set_context(p_event, kv.key.utf8(), kv.value);
	}
}

sentry_value_t handle_before_send(sentry_value_t event, void *hint, void *closure) {
	inject_contexts(event);
	return event;
}

sentry_value_t handle_before_crash(const sentry_ucontext_t *uctx, sentry_value_t event, void *closure) {
	inject_contexts(event);
	return event;
}

inline String _uuid_as_string(sentry_uuid_t p_uuid) {
	char str[37];
	sentry_uuid_as_string(&p_uuid, str);
	return str;
}

sentry_level_t _level_as_native(sentry::Level p_level) {
	switch (p_level) {
		case sentry::Level::LEVEL_DEBUG:
			return SENTRY_LEVEL_DEBUG;
		case sentry::Level::LEVEL_INFO:
			return SENTRY_LEVEL_INFO;
		case sentry::Level::LEVEL_WARNING:
			return SENTRY_LEVEL_WARNING;
		case sentry::Level::LEVEL_ERROR:
			return SENTRY_LEVEL_ERROR;
		case sentry::Level::LEVEL_FATAL:
			return SENTRY_LEVEL_FATAL;
		default:
			ERR_FAIL_V_MSG(SENTRY_LEVEL_ERROR, "SentrySDK: Internal error - unexpected level value. Please open an issue.");
	}
}

} // unnamed namespace

namespace sentry {

void NativeSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(p_key.is_empty());
	sentry_set_context(p_key.utf8(), sentry::native::variant_to_sentry_value(p_value));
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
	sentry_value_set_by_key(crumb, "data", sentry::native::variant_to_sentry_value(p_data));
	sentry_add_breadcrumb(crumb);
}

String NativeSDK::capture_message(const String &p_message, Level p_level, const String &p_logger) {
	sentry_value_t event = sentry_value_new_message_event(
			_level_as_native(p_level),
			p_logger.utf8().get_data(),
			p_message.utf8().get_data());
	last_uuid = sentry_capture_event(event);
	return _uuid_as_string(last_uuid);
}

String NativeSDK::get_last_event_id() {
	return _uuid_as_string(last_uuid);
}

String NativeSDK::capture_error(const String &p_type, const String &p_value, Level p_level, const Vector<StackFrame> &p_frames) {
	sentry_value_t event = sentry_value_new_event();
	sentry_value_set_by_key(event, "level",
			sentry_value_new_string(sentry::level_as_cstring(p_level)));

	sentry_value_t exception = sentry_value_new_exception(p_type.utf8(), p_value.utf8());
	sentry_value_t stack_trace = sentry_value_new_object();

	sentry_value_t frames = sentry_value_new_list();
	for (const StackFrame &frame : p_frames) {
		sentry_value_t sentry_frame = sentry_value_new_object();
		sentry_value_set_by_key(sentry_frame, "filename", sentry_value_new_string(frame.filename.utf8()));
		sentry_value_set_by_key(sentry_frame, "function", sentry_value_new_string(frame.function.utf8()));
		sentry_value_set_by_key(sentry_frame, "lineno", sentry_value_new_int32(frame.lineno));
		if (!frame.context_line.is_empty()) {
			sentry_value_set_by_key(sentry_frame, "context_line", sentry_value_new_string(frame.context_line.utf8()));
			sentry_value_set_by_key(sentry_frame, "pre_context", sentry::native::strings_to_sentry_list(frame.pre_context));
			sentry_value_set_by_key(sentry_frame, "post_context", sentry::native::strings_to_sentry_list(frame.post_context));
		}
		sentry_value_append(frames, sentry_frame);
	}

	sentry_value_set_by_key(stack_trace, "frames", frames);
	sentry_value_set_by_key(exception, "stacktrace", stack_trace);
	sentry_event_add_exception(event, exception);
	last_uuid = sentry_capture_event(event);
	return _uuid_as_string(last_uuid);
}

void NativeSDK::initialize() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	sentry_options_t *options = sentry_options_new();
	sentry_options_set_dsn(options, SentryOptions::get_singleton()->get_dsn());

	// Establish handler path.
	String handler_fn;
#if defined(LINUX_ENABLED) || defined(MACOS_ENABLED)
	handler_fn = "crashpad_handler";
#elif WINDOWS_ENABLED
	handler_fn = "crashpad_handler.exe";
#else
	ERR_PRINT("Sentry: Internal Error: NativeSDK should not be initialized on an unsupported platform (this should not happen).");
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

	// Hooks.
	sentry_options_set_before_send(options, handle_before_send, NULL);
	sentry_options_set_on_crash(options, handle_before_crash, NULL);

	sentry_init(options);
}

NativeSDK::~NativeSDK() {
	sentry_close();
}

} //namespace sentry
