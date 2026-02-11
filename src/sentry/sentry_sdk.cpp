#include "sentry_sdk.h"

#include "gen/sdk_version.gen.h"
#include "sentry/common_defs.h"
#include "sentry/contexts.h"
#include "sentry/disabled/disabled_sdk.h"
#include "sentry/godot_singletons.h"
#include "sentry/logging/print.h"
#include "sentry/processing/screenshot_processor.h"
#include "sentry/processing/view_hierarchy_processor.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_options.h"
#include "sentry/util/simple_bind.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/mutex_lock.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#ifdef SDK_NATIVE
#include "sentry/native/native_sdk.h"
#elif SDK_ANDROID
#include "sentry/android/android_sdk.h"
#elif SDK_COCOA
#include "sentry/cocoa/cocoa_sdk.h"
#elif SDK_JAVASCRIPT
#include "sentry/javascript/javascript_sdk.h"
#endif

using namespace godot;
using namespace sentry;

namespace {

void _verify_project_settings() {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	ERR_FAIL_NULL(ps);
	Ref<SentryOptions> options = SENTRY_OPTIONS();
	ERR_FAIL_COND(options.is_null());
	ERR_FAIL_NULL(Engine::get_singleton());

	if (!ps->get_setting("debug/settings/gdscript/always_track_call_stacks")) {
		if (Engine::get_singleton()->is_editor_hint()) {
			ps->set_setting("debug/settings/gdscript/always_track_call_stacks", true);
			ps->save();
			print_line("Sentry: Automatically enabled call stack tracking in project settings. This setting is required for Sentry to capture GDScript stack traces.");
		} else {
			ERR_PRINT("Sentry: Please enable `debug/settings/gdscript/always_track_call_stacks` in your Project Settings. This is required for supporting script stack traces.");
		}
	}
	if (options->is_logger_include_variables_enabled() &&
			!ps->get_setting("debug/settings/gdscript/always_track_local_variables")) {
		if (Engine::get_singleton()->is_editor_hint()) {
			ps->set_setting("debug/settings/gdscript/always_track_local_variables", true);
			ps->save();
			print_line("Sentry: Automatically enabled local variable tracking in project settings. This setting is required for Sentry to capture local variables in backtraces.");
		} else {
			ERR_PRINT("Sentry: Please enable `debug/settings/gdscript/always_track_local_variables` in your Project Settings. This is required to include local variables in backtraces.");
		}
	}

	if (options->is_attach_log_enabled()) {
#if defined(LINUX_ENABLED) || defined(WINDOWS_ENABLED) || defined(MACOS_ENABLED)
		if (!ps->get_setting("debug/file_logging/enable_file_logging.pc") &&
				!ps->get_setting("debug/file_logging/enable_file_logging")) {
#else
		if (!ps->get_setting("debug/file_logging/enable_file_logging")) {
#endif
			ERR_PRINT("Sentry: Please enable File Logging in your Project Settings if you want to include log files with Sentry events or disable attaching logs in your Sentry options.");
		}
	}
}

void _fix_unix_executable_permissions(const String &p_path) {
	if (!FileAccess::file_exists(p_path)) {
		return;
	}

	BitField<FileAccess::UnixPermissionFlags> perm = FileAccess::get_unix_permissions(p_path);
	BitField<FileAccess::UnixPermissionFlags> new_perm = perm;

	if (!perm.has_flag(FileAccess::UNIX_EXECUTE_OWNER)) {
		new_perm.set_flag(FileAccess::UNIX_EXECUTE_OWNER);
	}
	if (!perm.has_flag(FileAccess::UNIX_EXECUTE_GROUP)) {
		new_perm.set_flag(FileAccess::UNIX_EXECUTE_GROUP);
	}
	if (!perm.has_flag(FileAccess::UNIX_EXECUTE_OTHER)) {
		new_perm.set_flag(FileAccess::UNIX_EXECUTE_OTHER);
	}

	if (perm != new_perm) {
		godot::Error err = FileAccess::set_unix_permissions(p_path, new_perm);
		if (err != OK && err != ERR_UNAVAILABLE) {
			sentry::logging::print_error(vformat("Failed to set executable permissions for %s (error code %d)", p_path, err));
		}
	}
}

} // unnamed namespace

namespace sentry {

SentrySDK *SentrySDK::singleton = nullptr;

void SentrySDK::create_singleton() {
	ERR_FAIL_NULL(Engine::get_singleton());
	singleton = memnew(SentrySDK);
	Engine::get_singleton()->register_singleton("SentrySDK", SentrySDK::get_singleton());
}

void SentrySDK::destroy_singleton() {
	ERR_FAIL_NULL(Engine::get_singleton());
	if (!singleton) {
		return;
	}
	Engine::get_singleton()->unregister_singleton("SentrySDK");
	memdelete(singleton);
	singleton = nullptr;
}

void SentrySDK::init(const Callable &p_configuration_callback) {
	ERR_FAIL_COND_MSG(internal_sdk->is_enabled(), "Attempted to initialize SentrySDK that is already initialized");

#if SDK_ANDROID
	if (OS::get_singleton()->has_feature("editor")) {
		ERR_FAIL_MSG("Initializing in Android editor is not supported!");
		return;
	}
#endif

	// Fresh options from project settings.
	options = SentryOptions::create_from_project_settings();

	// Add built-in event processors.
	if (options->is_attach_screenshot_enabled()) {
		options->add_event_processor(memnew(ScreenshotProcessor));
	}
	if (options->is_attach_scene_tree_enabled()) {
		options->add_event_processor(memnew(ViewHierarchyProcessor));
	}

	sentry::logging::print_debug("Initializing Sentry SDK");
	internal_sdk->init(_get_global_attachments(), p_configuration_callback);

	if (internal_sdk->is_enabled()) {
		if (is_auto_initializing) {
			// Delay contexts initialization until engine singletons are ready during early initialization.
			callable_mp(this, &SentrySDK::_init_contexts).call_deferred();
		} else {
			// TODO: move this into sentry::contexts
			_init_contexts();
		}

		if (options->is_logger_enabled()) {
			if (godot_logger.is_null()) {
				godot_logger.instantiate();
			}
			OS::get_singleton()->add_logger(godot_logger);
		}
	}
}

void SentrySDK::close() {
	if (internal_sdk->is_enabled()) {
		sentry::logging::print_debug("Shutting down Sentry SDK");
		if (godot_logger.is_valid()) {
			OS::get_singleton()->remove_logger(godot_logger);
			godot_logger.unref();
		}
		internal_sdk->close();
	}
}

String SentrySDK::capture_message(const String &p_message, Level p_level) {
	return internal_sdk->capture_message(p_message, p_level);
}

void SentrySDK::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ERR_FAIL_COND_MSG(p_breadcrumb.is_null(), "Sentry: Can't add null breadcrumb.");
	internal_sdk->add_breadcrumb(p_breadcrumb);
}

String SentrySDK::get_last_event_id() const {
	return internal_sdk->get_last_event_id();
}

Ref<SentryEvent> SentrySDK::create_event() const {
	return internal_sdk->create_event();
}

String SentrySDK::capture_event(const Ref<SentryEvent> &p_event) {
	ERR_FAIL_COND_V_MSG(p_event.is_null(), "", "Sentry: Can't capture event - event object is null.");
	return internal_sdk->capture_event(p_event);
}

void SentrySDK::capture_feedback(const Ref<SentryFeedback> &p_feedback) {
	ERR_FAIL_COND_MSG(p_feedback.is_null(), "Sentry: Can't capture feedback - feedback object is null.");
	ERR_FAIL_COND_MSG(p_feedback->get_message().is_empty(), "Sentry: Can't capture feedback - feedback message is empty.");
	if (p_feedback->get_message().length() > 4096) {
		WARN_PRINT("Sentry: Feedback message is too long (max 4096 characters).");
	}
	return internal_sdk->capture_feedback(p_feedback);
}

void SentrySDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	ERR_FAIL_COND_MSG(p_attachment.is_null(), "Sentry: Can't add null attachment.");
	internal_sdk->add_attachment(p_attachment);
}

void SentrySDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set tag with an empty key.");
	internal_sdk->set_tag(p_key, p_value);
}

void SentrySDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't remove tag with an empty key.");
	internal_sdk->remove_tag(p_key);
}

void SentrySDK::set_user(const Ref<SentryUser> &p_user) {
	internal_sdk->set_user(p_user);
}

void SentrySDK::remove_user() {
	internal_sdk->remove_user();
}

void SentrySDK::set_context(const godot::String &p_key, const godot::Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set context with an empty key.");
	internal_sdk->set_context(p_key, p_value);
}

void SentrySDK::_init_contexts() {
	sentry::logging::print_debug("initializing contexts");

	// Mark Godot engine singletons as safe to access.
	sentry::godot_singletons::mark_as_ready();

#if defined(SDK_NATIVE) || defined(SDK_JAVASCRIPT)
	internal_sdk->set_context("device", sentry::contexts::make_device_context(runtime_config));
	internal_sdk->set_context("app", sentry::contexts::make_app_context());
#endif

	internal_sdk->set_context("culture", sentry::contexts::make_culture_context());
	internal_sdk->set_context("gpu", sentry::contexts::make_gpu_context());

	// Custom contexts.
	internal_sdk->set_context("display", sentry::contexts::make_display_context());
	internal_sdk->set_context("godot_engine", sentry::contexts::make_godot_engine_context());
	internal_sdk->set_context("environment", sentry::contexts::make_environment_context());
}

PackedStringArray SentrySDK::_get_global_attachments() {
	PackedStringArray attachments;

	// Attach LOG file.
	if (options->is_attach_log_enabled()) {
		String log_path = ProjectSettings::get_singleton()->get_setting("debug/file_logging/log_path");
		if (FileAccess::file_exists(log_path)) {
			log_path = log_path.replace("user://", OS::get_singleton()->get_user_data_dir() + "/");
			attachments.append(log_path);
		} else {
			ERR_PRINT("Sentry: Log file not found. Make sure \"debug/file_logging/enable_file_logging\" is turned ON in the Project Settings.");
		}
	}

	// Attach screenshot.
	if (options->is_attach_screenshot_enabled()) {
		String screenshot_path = OS::get_singleton()->get_user_data_dir().path_join(SENTRY_SCREENSHOT_FN);
		DirAccess::remove_absolute(screenshot_path);
		attachments.append(screenshot_path);
	}

	// Attach view hierarchy (aka scene tree info).
	if (options->is_attach_scene_tree_enabled()) {
		String vh_path = OS::get_singleton()->get_user_data_dir().path_join(SENTRY_VIEW_HIERARCHY_FN);
		DirAccess::remove_absolute(vh_path);
		attachments.append(vh_path);
	}

	return attachments;
}

void SentrySDK::_auto_initialize() {
	sentry::logging::print_debug("starting Sentry SDK version " + String(SENTRY_GODOT_SDK_VERSION));

	bool should_enable = true;

	if (!options->is_auto_init_enabled()) {
		should_enable = false;
		sentry::logging::print_debug("Automatic initialization is disabled in the project settings.");
	}

	if (Engine::get_singleton()->is_editor_hint()) {
		should_enable = false;
		sentry::logging::print_debug("Automatic initialization is disabled in the editor.");
	}

	if (!Engine::get_singleton()->is_editor_hint() && OS::get_singleton()->has_feature("editor") && options->should_skip_auto_init_on_editor_play()) {
		should_enable = false;
		sentry::logging::print_debug("Automatic initialization is disabled when project is played from the editor. Tip: This can be changed in the project settings.");
	}

	if (options->get_dsn().is_empty()) {
		should_enable = false;
		sentry::logging::print_debug("Automatic initialization is disabled because no DSN was provided. Tip: You can obtain a DSN from Sentry's dashboard and add it in the project settings.");
	}

	if (!should_enable) {
		sentry::logging::print_info("Automatic initialization is disabled! Operations with Sentry SDK will result in no-ops.");
		return;
	}

	sentry::logging::print_debug("Proceeding with automatic initialization.");

	is_auto_initializing = true;
	init();
	is_auto_initializing = false;
}

void SentrySDK::_demo_helper_crash_app() {
	char *ptr = (char *)1;
	sentry::logging::print_fatal("Crash by access violation ", ptr); // this is going to crash the app
}

void SentrySDK::prepare_and_auto_initialize() {
	// Create platform-specific SDK backend (replaces the default DisabledSDK).
#ifdef SDK_NATIVE
	internal_sdk = std::make_unique<sentry::native::NativeSDK>();
#elif SDK_ANDROID
	if (unlikely(OS::get_singleton()->has_feature("editor"))) {
		sentry::logging::print_debug("Sentry SDK is disabled in Android editor mode (only supported in exported Android projects)");
		// internal_sdk stays DisabledSDK
	} else {
		auto sdk = std::make_unique<sentry::android::AndroidSDK>();
		if (sdk->has_android_plugin()) {
			internal_sdk = std::move(sdk);
		} else {
			sentry::logging::print_error("Failed to initialize on Android. Disabling Sentry SDK...");
			// internal_sdk stays DisabledSDK
		}
	}
#elif SDK_COCOA
	internal_sdk = std::make_unique<sentry::cocoa::CocoaSDK>();
#elif SDK_JAVASCRIPT
	internal_sdk = std::make_unique<sentry::javascript::JavaScriptSDK>();
#else
	sentry::logging::print_debug("This is an unsupported platform. Disabling Sentry SDK...");
	// internal_sdk stays DisabledSDK
#endif

	// Load the runtime configuration from the user's data directory.
	runtime_config.instantiate();
	runtime_config->load_file(OS::get_singleton()->get_user_data_dir() + "/sentry.dat");

	// Verify project settings and notify user via errors if there are any issues (deferred).
	callable_mp_static(_verify_project_settings).call_deferred();

#if defined(LINUX_ENABLED) || defined(MACOS_ENABLED)
	// Fix crashpad handler executable bit permissions on Unix platforms if the
	// user extracts the distribution archive without preserving such permissions.
	if (OS::get_singleton()->is_debug_build()) {
		_fix_unix_executable_permissions("res://addons/sentry/bin/macos/crashpad_handler");
		_fix_unix_executable_permissions("res://addons/sentry/bin/linux/x86_64/crashpad_handler");
		_fix_unix_executable_permissions("res://addons/sentry/bin/linux/x86_32/crashpad_handler");
		_fix_unix_executable_permissions("res://addons/sentry/bin/linux/arm64/crashpad_handler");
		_fix_unix_executable_permissions("res://addons/sentry/bin/linux/arm32/crashpad_handler");
	}
#endif

	if (internal_sdk->get_capabilities().has_flag(InternalSDK::SUPPORTS_EARLY_INIT)) {
		_auto_initialize();
	} else {
		// Defer automatic initialization when the underlying SDK cannot be initialized early.
		callable_mp(this, &SentrySDK::_auto_initialize).call_deferred();
	}
}

void SentrySDK::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PREDELETE: {
			if (godot_logger.is_valid()) {
				OS::get_singleton()->remove_logger(godot_logger);
				godot_logger.unref();
			}
		} break;
	}
}

void SentrySDK::_bind_methods() {
	BIND_ENUM_CONSTANT(LEVEL_DEBUG);
	BIND_ENUM_CONSTANT(LEVEL_INFO);
	BIND_ENUM_CONSTANT(LEVEL_WARNING);
	BIND_ENUM_CONSTANT(LEVEL_ERROR);
	BIND_ENUM_CONSTANT(LEVEL_FATAL);

	ClassDB::bind_method(D_METHOD("init", "configuration_callback"), &SentrySDK::init, DEFVAL(Callable()));
	ClassDB::bind_method(D_METHOD("close"), &SentrySDK::close);
	ClassDB::bind_method(D_METHOD("is_enabled"), &SentrySDK::is_enabled);
	ClassDB::bind_method(D_METHOD("add_breadcrumb", "breadcrumb"), &SentrySDK::add_breadcrumb);
	ClassDB::bind_method(D_METHOD("capture_message", "message", "level"), &SentrySDK::capture_message, DEFVAL(LEVEL_INFO));
	ClassDB::bind_method(D_METHOD("get_last_event_id"), &SentrySDK::get_last_event_id);
	ClassDB::bind_method(D_METHOD("set_context", "key", "value"), &SentrySDK::set_context);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &SentrySDK::set_tag);
	ClassDB::bind_method(D_METHOD("remove_tag", "key"), &SentrySDK::remove_tag);
	ClassDB::bind_method(D_METHOD("set_user", "user"), &SentrySDK::set_user);
	ClassDB::bind_method(D_METHOD("remove_user"), &SentrySDK::remove_user);
	ClassDB::bind_method(D_METHOD("create_event"), &SentrySDK::create_event);
	ClassDB::bind_method(D_METHOD("capture_event", "event"), &SentrySDK::capture_event);
	ClassDB::bind_method(D_METHOD("capture_feedback", "feedback"), &SentrySDK::capture_feedback);
	ClassDB::bind_method(D_METHOD("add_attachment", "attachment"), &SentrySDK::add_attachment);

	// Hidden API methods -- used in testing.
	ClassDB::bind_method(D_METHOD("_set_before_send", "callable"), &SentrySDK::set_before_send);
	ClassDB::bind_method(D_METHOD("_unset_before_send"), &SentrySDK::unset_before_send);
	ClassDB::bind_method(D_METHOD("_get_before_send"), &SentrySDK::get_before_send);
	ClassDB::bind_method(D_METHOD("_demo_helper_crash_app"), &SentrySDK::_demo_helper_crash_app);

	BIND_PROPERTY_READONLY(SentrySDK, PropertyInfo(Variant::OBJECT, "logger", PROPERTY_HINT_TYPE_STRING, "SentryLogger", PROPERTY_USAGE_NONE), get_logger);
}

SentrySDK::SentrySDK() {
	ERR_FAIL_NULL(OS::get_singleton());

	options = SentryOptions::create_from_project_settings();
	logger = memnew(SentryLogger);
	internal_sdk = std::make_unique<DisabledSDK>();
}

SentrySDK::~SentrySDK() {
	internal_sdk.reset();

	singleton = nullptr;

	memdelete(logger);
	logger = nullptr;
}

} // namespace sentry
