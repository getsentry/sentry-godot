#include "native_sdk.h"

#include "sentry.h"
#include "sentry/common_defs.h"
#include "sentry/level.h"
#include "sentry/native/native_breadcrumb.h"
#include "sentry/native/native_event.h"
#include "sentry/native/native_util.h"
#include "sentry/processing/process_event.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_options.h"
#include "sentry/util/print.h"
#include "sentry/util/screenshot.h"

#include <cstdio>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace {

using NativeEvent = sentry::native::NativeEvent;

sentry_value_t _handle_before_send(sentry_value_t event, void *hint, void *closure) {
	Ref<NativeEvent> event_obj = memnew(NativeEvent(event, false));
	Ref<NativeEvent> processed = sentry::process_event(event_obj);

	if (unlikely(processed.is_null())) {
		// Discard event.
		sentry_value_decref(event);
		return sentry_value_new_null();
	} else {
		return event;
	}
}

sentry_value_t _handle_on_crash(const sentry_ucontext_t *uctx, sentry_value_t event, void *closure) {
	Ref<NativeEvent> event_obj = memnew(NativeEvent(event, true));
	Ref<NativeEvent> processed = sentry::process_event(event_obj);

	if (unlikely(processed.is_null())) {
		// Discard event.
		sentry_value_decref(event);
		return sentry_value_new_null();
	} else {
		return event;
	}
}

inline bool _cstring_begins_with(const char *str, size_t str_len, const char *prefix, size_t prefix_len) {
	if (str_len < prefix_len) {
		return false;
	}
	return strncmp(str, prefix, prefix_len) == 0;
}

inline bool _cstring_ends_with(const char *str, size_t str_len, const char *suffix, size_t suffix_len) {
	if (str_len < suffix_len) {
		return false;
	}
	return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

void _log_native_message(sentry_level_t level, const char *message, va_list args, void *userdata) {
	char initial_buffer[512];
	va_list args_copy;
	va_copy(args_copy, args);

	int required = vsnprintf(initial_buffer, sizeof(initial_buffer), message, args);
	if (required < 0) {
		va_end(args_copy);
		ERR_FAIL_MSG("Sentry: Error fomatting message");
	}

	char *buffer = initial_buffer;
	if (required >= sizeof(initial_buffer)) {
		buffer = (char *)malloc(required + 1);
		if (buffer) {
			int new_required = vsnprintf(buffer, required + 1, message, args_copy);
			if (new_required < 0) {
				free(buffer);
				buffer = initial_buffer;
			}
		} else {
			buffer = initial_buffer;
		}
	}
	va_end(args_copy);

	bool accepted = true;

	// Filter out warnings about missing attachment files that may not exist in some scenarios.
	constexpr char warning_prefix[] = "failed to read envelope item from";
	constexpr size_t warning_prefix_len = sizeof(warning_prefix) - 1;
	if (_cstring_begins_with(buffer, required, warning_prefix, warning_prefix_len)) {
		constexpr char screenshot_suffix[] = "screenshot.jpg\"";
		constexpr size_t screenshot_len = sizeof(screenshot_suffix) - 1;
		constexpr char vh_suffix[] = "view-hierarchy.json\"";
		constexpr size_t vh_len = sizeof(vh_suffix) - 1;
		if (_cstring_ends_with(buffer, required, screenshot_suffix, screenshot_len) ||
				_cstring_ends_with(buffer, required, vh_suffix, vh_len)) {
			accepted = false;
		}
	}

	if (accepted) {
		sentry::util::print(sentry::native::native_to_level(level), String(buffer));
	}

	if (buffer != initial_buffer) {
		free(buffer);
	}
}

inline String _uuid_as_string(sentry_uuid_t p_uuid) {
	char str[37];
	sentry_uuid_as_string(&p_uuid, str);
	return str;
}

} // unnamed namespace

namespace sentry::native {

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
	if (p_user.is_valid()) {
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
	} else {
		remove_user();
	}
}

void NativeSDK::remove_user() {
	sentry_remove_user();
}

Ref<SentryBreadcrumb> NativeSDK::create_breadcrumb() {
	return memnew(NativeBreadcrumb);
}

void NativeSDK::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ERR_FAIL_COND_MSG(p_breadcrumb.is_null(), "Sentry: Can't add breadcrumb - breadcrumb object is null.");
	NativeBreadcrumb *crumb = Object::cast_to<NativeBreadcrumb>(p_breadcrumb.ptr());
	ERR_FAIL_NULL(crumb);
	sentry_value_t native_crumb = crumb->get_native_breadcrumb();
	sentry_value_incref(native_crumb); // give ownership to native
	sentry_add_breadcrumb(native_crumb);
}

String NativeSDK::capture_message(const String &p_message, Level p_level) {
	sentry_value_t event = sentry_value_new_message_event(
			native::level_to_native(p_level),
			"", // logger
			p_message.utf8().get_data());

	sentry_uuid_t uuid = sentry_capture_event(event);

	last_uuid_mutex->lock();
	last_uuid = uuid;
	last_uuid_mutex->unlock();

	return _uuid_as_string(uuid);
}

String NativeSDK::get_last_event_id() {
	last_uuid_mutex->lock();
	String uuid_str = _uuid_as_string(last_uuid);
	last_uuid_mutex->unlock();

	return uuid_str;
}

Ref<SentryEvent> NativeSDK::create_event() {
	sentry_value_t event_value = sentry_value_new_event();
	Ref<SentryEvent> event = memnew(NativeEvent(event_value, false));
	return event;
}

String NativeSDK::capture_event(const Ref<SentryEvent> &p_event) {
	ERR_FAIL_COND_V_MSG(p_event.is_null(), _uuid_as_string(sentry_uuid_nil()), "Sentry: Can't capture event - event object is null.");
	NativeEvent *native_event = Object::cast_to<NativeEvent>(p_event.ptr());
	ERR_FAIL_NULL_V(native_event, _uuid_as_string(sentry_uuid_nil())); // Sanity check - this should never happen.
	sentry_value_t event = native_event->get_native_value();
	sentry_value_incref(event); // Keep ownership.
	sentry_uuid_t uuid = sentry_capture_event(event);

	last_uuid_mutex->lock();
	last_uuid = uuid;
	last_uuid_mutex->unlock();

	return _uuid_as_string(uuid);
}

void NativeSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	ERR_FAIL_COND_MSG(p_attachment.is_null(), "Sentry: Can't add null attachment.");
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	sentry_attachment_t *native_attachment = nullptr;

	if (!p_attachment->get_path().is_empty()) {
		String absolute_path = ProjectSettings::get_singleton()->globalize_path(p_attachment->get_path());
		sentry::util::print_debug(vformat("attaching file: %s", absolute_path));

		native_attachment = sentry_attach_file(absolute_path.utf8());

		ERR_FAIL_NULL_MSG(native_attachment, vformat("Sentry: Failed to attach file: %s", absolute_path));

		if (!p_attachment->get_filename().is_empty()) {
			sentry_attachment_set_filename(native_attachment, p_attachment->get_filename().utf8());
		}

	} else {
		PackedByteArray bytes = p_attachment->get_bytes();
		ERR_FAIL_COND_MSG(bytes.is_empty(), "Sentry: Can't add attachment with empty bytes and no file path.");

		sentry::util::print_debug(vformat("attaching bytes with filename: %s", p_attachment->get_filename()));

		native_attachment = sentry_attach_bytes(
				reinterpret_cast<const char *>(bytes.ptr()),
				bytes.size(),
				p_attachment->get_filename().utf8());

		ERR_FAIL_NULL_MSG(native_attachment, vformat("Sentry: Failed to attach bytes with filename: %s", p_attachment->get_filename()));
	}

	p_attachment->set_native_attachment(native_attachment);

	if (!p_attachment->get_content_type().is_empty()) {
		sentry_attachment_set_content_type(native_attachment, p_attachment->get_content_type().utf8());
	}
}

void NativeSDK::init(const PackedStringArray &p_global_attachments, const Callable &p_configuration_callback) {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	if (p_configuration_callback.is_valid()) {
		p_configuration_callback.call(SentryOptions::get_singleton());
	}

	sentry_options_t *options = sentry_options_new();

	sentry_options_set_dsn(options, SentryOptions::get_singleton()->get_dsn().utf8());
	sentry_options_set_database_path(options, (OS::get_singleton()->get_user_data_dir() + "/sentry").utf8());
	sentry_options_set_debug(options, SentryOptions::get_singleton()->is_debug_enabled());
	sentry_options_set_release(options, SentryOptions::get_singleton()->get_release().utf8());
	sentry_options_set_dist(options, SentryOptions::get_singleton()->get_dist().utf8());
	sentry_options_set_environment(options, SentryOptions::get_singleton()->get_environment().utf8());
	sentry_options_set_sample_rate(options, SentryOptions::get_singleton()->get_sample_rate());
	sentry_options_set_max_breadcrumbs(options, SentryOptions::get_singleton()->get_max_breadcrumbs());
	sentry_options_set_sdk_name(options, "sentry.native.godot");
	sentry_options_set_logger_enabled_when_crashed(options, false);

	// Establish handler path.
	String handler_fn;
	String platform_dir;
	String export_subdir;
#ifdef LINUX_ENABLED
	handler_fn = "crashpad_handler";
	platform_dir = "linux/" + Engine::get_singleton()->get_architecture_name();
#elif MACOS_ENABLED
	handler_fn = "crashpad_handler";
	platform_dir = "macos";
	export_subdir = "../Frameworks";
#elif WINDOWS_ENABLED
	handler_fn = "crashpad_handler.exe";
	platform_dir = "windows/" + Engine::get_singleton()->get_architecture_name();
#else
	ERR_PRINT("Sentry: Internal Error: NativeSDK should not be initialized on an unsupported platform (this should not happen).");
#endif
	String exe_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	String handler_path = exe_dir.path_join(export_subdir).path_join(handler_fn);
	if (!FileAccess::file_exists(handler_path)) {
		const String addon_bin_dir = "res://addons/sentry/bin/";
		handler_path = ProjectSettings::get_singleton()->globalize_path(
				addon_bin_dir.path_join(platform_dir).path_join(handler_fn));
	}
	if (FileAccess::file_exists(handler_path)) {
		sentry_options_set_handler_path(options, handler_path.utf8());
	} else {
		ERR_PRINT(vformat("Sentry: Failed to locate crash handler (crashpad) - backend disabled (%s)", handler_path));
		sentry_options_set_backend(options, NULL);
	}

	for (const String &path : p_global_attachments) {
		sentry::util::print_debug("adding attachment \"", path, "\"");
		if (path.ends_with(SENTRY_VIEW_HIERARCHY_FN)) {
			sentry_options_add_view_hierarchy(options, path.utf8());
		} else {
			sentry_options_add_attachment(options, path.utf8());
		}
	}

	// Hooks.
	sentry_options_set_before_send(options, _handle_before_send, NULL);
	sentry_options_set_on_crash(options, _handle_on_crash, NULL);
	sentry_options_set_logger(options, _log_native_message, NULL);

	int err = sentry_init(options);
	initialized = (err == 0);

	if (is_enabled()) {
		set_user(SentryUser::create_default());
	} else {
		ERR_PRINT("Sentry: Failed to initialize native SDK. Error code: " + itos(err));
	}
}

void NativeSDK::close() {
	int err = sentry_close();
	initialized = false;

	if (err != 0) {
		ERR_PRINT("Sentry: Failed to close native SDK cleanly. Error code: " + itos(err));
	}
}

bool NativeSDK::is_enabled() const {
	return initialized;
}

NativeSDK::NativeSDK() {
	last_uuid_mutex.instantiate();
	last_uuid = sentry_uuid_nil();
}

NativeSDK::~NativeSDK() {
	if (is_enabled()) {
		close();
	}
}

} //namespace sentry::native
