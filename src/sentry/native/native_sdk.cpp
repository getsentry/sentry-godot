#include "native_sdk.h"

#include "sentry.h"
#include "sentry/contexts.h"
#include "sentry/level.h"
#include "sentry/native/native_event.h"
#include "sentry/native/native_util.h"
#include "sentry/util/print.h"
#include "sentry/util/screenshot.h"
#include "sentry/view_hierarchy.h"
#include "sentry_options.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#define _SCREENSHOT_FN "screenshot.png"
#define _SCENE_TREE_FN "view-hierarchy.json"

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

inline void _save_screenshot() {
	String screenshot_path = "user://" _SCREENSHOT_FN;
	DirAccess::remove_absolute(screenshot_path);

	if (!DisplayServer::get_singleton() || DisplayServer::get_singleton()->get_name() == "headless") {
		return;
	}

	PackedByteArray buffer = sentry::util::take_screenshot();
	Ref<FileAccess> f = FileAccess::open(screenshot_path, FileAccess::WRITE);
	if (f.is_valid()) {
		f->store_buffer(buffer);
		f->flush();
		f->close();
	} else {
		sentry::util::print_error("Failed to save screenshot to file");
	}
}

inline void _save_view_hierarchy() {
	String path = "user://" _SCENE_TREE_FN;
	DirAccess::remove_absolute(path);
	String json_content = sentry::build_view_hierarchy_json();
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
	if (f.is_valid()) {
		f->store_string(json_content);
		f->flush();
		f->close();
	} else {
		sentry::util::print_error("Failed to save view hierarchy to file");
	}
}

inline void _inject_contexts(sentry_value_t p_event) {
	ERR_FAIL_COND(sentry_value_get_type(p_event) != SENTRY_VALUE_TYPE_OBJECT);

	HashMap<String, Dictionary> contexts = sentry::contexts::make_event_contexts();
	for (const auto &kv : contexts) {
		sentry_event_set_context(p_event, kv.key.utf8(), kv.value);
	}
}

sentry_value_t _handle_before_send(sentry_value_t event, void *hint, void *closure) {
	sentry::util::print_debug("handling before_send");
	_save_screenshot();
	_save_view_hierarchy();
	_inject_contexts(event);
	if (const Callable &before_send = SentryOptions::get_singleton()->get_before_send(); before_send.is_valid()) {
		Ref<NativeEvent> event_obj = memnew(NativeEvent(event));
		Ref<NativeEvent> processed = before_send.call(event_obj);
		ERR_FAIL_COND_V_MSG(processed.is_valid() && processed != event_obj, event, "Sentry: before_send callback must return the same event object or null.");
		if (processed.is_null()) {
			// Discard event.
			sentry::util::print_debug("event discarded by before_send callback: ", event_obj->get_id());
			sentry_value_decref(event);
			return sentry_value_new_null();
		}
		sentry::util::print_debug("event processed by before_send callback: ", event_obj->get_id());
	}
	return event;
}

sentry_value_t _handle_on_crash(const sentry_ucontext_t *uctx, sentry_value_t event, void *closure) {
	sentry::util::print_debug("handling on_crash");
	_save_screenshot();
	_save_view_hierarchy();
	_inject_contexts(event);
	if (const Callable &on_crash = SentryOptions::get_singleton()->get_on_crash(); on_crash.is_valid()) {
		Ref<NativeEvent> event_obj = memnew(NativeEvent(event));
		Ref<NativeEvent> processed = on_crash.call(event_obj);
		ERR_FAIL_COND_V_MSG(processed.is_valid() && processed != event_obj, event, "Sentry: on_crash callback must return the same event object or null.");
		if (processed.is_null()) {
			// Discard event.
			sentry::util::print_debug("event discarded by on_crash callback: ", event_obj->get_id());
			sentry_value_decref(event);
			return sentry_value_new_null();
		}
		sentry::util::print_debug("event processed by on_crash callback: ", event_obj->get_id());
	}
	return event;
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

	sentry::util::print(sentry::native::native_to_level(level), String(buffer));

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
			native::level_to_native(p_level),
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

Ref<SentryEvent> NativeSDK::create_event() {
	sentry_value_t event_value = sentry_value_new_event();
	Ref<SentryEvent> event = memnew(NativeEvent(event_value));
	return event;
}

String NativeSDK::capture_event(const Ref<SentryEvent> &p_event) {
	last_uuid = sentry_uuid_nil();
	ERR_FAIL_COND_V_MSG(p_event.is_null(), _uuid_as_string(last_uuid), "Sentry: Can't capture event - event object is null.");
	NativeEvent *native_event = Object::cast_to<NativeEvent>(p_event.ptr());
	ERR_FAIL_NULL_V(native_event, _uuid_as_string(last_uuid)); // Sanity check - this should never happen.
	sentry_value_t event = native_event->get_native_value();
	sentry_value_incref(event); // Keep ownership.
	last_uuid = sentry_capture_event(event);
	return _uuid_as_string(last_uuid);
}

void NativeSDK::initialize() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

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

	// Establish handler path.
	String handler_fn;
	String platform_dir;
	String export_subdir;
#ifdef LINUX_ENABLED
	handler_fn = "crashpad_handler";
	platform_dir = "linux";
#elif MACOS_ENABLED
	handler_fn = "crashpad_handler";
	platform_dir = "macos";
	export_subdir = "../Frameworks";
#elif WINDOWS_ENABLED
	handler_fn = "crashpad_handler.exe";
	platform_dir = "windows";
#else
	ERR_PRINT("Sentry: Internal Error: NativeSDK should not be initialized on an unsupported platform (this should not happen).");
#endif
	String exe_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	String handler_path = exe_dir.path_join(export_subdir).path_join(handler_fn);
	if (!FileAccess::file_exists(handler_path)) {
		const String addon_bin_dir = "res://addons/sentrysdk/bin/";
		handler_path = ProjectSettings::get_singleton()->globalize_path(
				addon_bin_dir.path_join(platform_dir).path_join(handler_fn));
	}
	if (FileAccess::file_exists(handler_path)) {
		sentry_options_set_handler_path(options, handler_path.utf8());
	} else {
		ERR_PRINT(vformat("Sentry: Failed to locate crash handler (crashpad) - backend disabled (%s)", handler_path));
		sentry_options_set_backend(options, NULL);
	}

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

	// Attach screenshot.
	if (SentryOptions::get_singleton()->is_attach_screenshot_enabled()) {
		String screenshot_path = OS::get_singleton()->get_user_data_dir().path_join(_SCREENSHOT_FN);
		sentry_options_add_attachment(options, screenshot_path.utf8());
	}

	// Attach scene tree JSON.
	String scene_tree_json_path = OS::get_singleton()->get_user_data_dir().path_join(_SCENE_TREE_FN);
	sentry_options_add_attachment(options, scene_tree_json_path.utf8());

	// Hooks.
	sentry_options_set_before_send(options, _handle_before_send, NULL);
	sentry_options_set_on_crash(options, _handle_on_crash, NULL);
	sentry_options_set_logger(options, _log_native_message, NULL);

	int err = sentry_init(options);
	initialized = (err == 0);

	if (err != 0) {
		ERR_PRINT("Sentry: Failed to initialize native SDK. Error code: " + itos(err));
	}
}

NativeSDK::~NativeSDK() {
	sentry_close();
}

} //namespace sentry
