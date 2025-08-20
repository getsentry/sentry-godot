#include "sentry_sdk.h"

#include "gen/sdk_version.gen.h"
#include "sentry/common_defs.h"
#include "sentry/contexts.h"
#include "sentry/disabled/disabled_sdk.h"
#include "sentry/processing/screenshot_processor.h"
#include "sentry/processing/view_hierarchy_processor.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_options.h"
#include "sentry/util/print.h"

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
#endif

using namespace godot;
using namespace sentry;

namespace {

void _verify_project_settings() {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	ERR_FAIL_NULL(ps);
	Ref<SentryOptions> options = SentryOptions::get_singleton();
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
			sentry::util::print_error(vformat("Failed to set executable permissions for %s (error code %d)", p_path, err));
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

void SentrySDK::init() {
	ERR_FAIL_COND_MSG(internal_sdk->is_enabled(), "Attempted to initialize SentrySDK that is already initialized");

	sentry::util::print_debug("Initializing Sentry SDK");
	internal_sdk->init(_get_global_attachments());

	if (internal_sdk->is_enabled()) {
		if (sentry::contexts::should_delay_contexts()) {
			// Delay contexts initialization until engine singletons are ready during early initialization.
			callable_mp(this, &SentrySDK::_init_contexts).call_deferred();
		} else {
			_init_contexts();
		}

		if (SentryOptions::get_singleton()->is_logger_enabled()) {
			if (logger.is_null()) {
				logger.instantiate();
			}
			OS::get_singleton()->add_logger(logger);
		}
	}
}

void SentrySDK::close() {
	if (internal_sdk->is_enabled()) {
		sentry::util::print_debug("Shutting down Sentry SDK");
		if (logger.is_valid()) {
			OS::get_singleton()->remove_logger(logger);
			logger.unref();
		}
		internal_sdk->close();
	}
}

String SentrySDK::capture_message(const String &p_message, Level p_level) {
	return internal_sdk->capture_message(p_message, p_level);
}

void SentrySDK::add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
		const String &p_type, const Dictionary &p_data) {
	internal_sdk->add_breadcrumb(p_message, p_category, p_level, p_type, p_data);
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
	MutexLock lock(*user_mutex.ptr());

	user = p_user;

	if (user.is_null()) {
		user.instantiate();
	}

	if (user->is_empty()) {
		internal_sdk->remove_user();
	} else {
		internal_sdk->set_user(p_user);
	}
}

Ref<SentryUser> SentrySDK::get_user() const {
	MutexLock lock(*user_mutex.ptr());
	return user.is_valid() ? user->duplicate() : nullptr;
}

void SentrySDK::remove_user() {
	MutexLock lock(*user_mutex.ptr());
	user.instantiate();
	internal_sdk->remove_user();
}

void SentrySDK::set_context(const godot::String &p_key, const godot::Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set context with an empty key.");
	internal_sdk->set_context(p_key, p_value);
}

void SentrySDK::_init_contexts() {
	sentry::util::print_debug("initializing contexts");

#ifdef SDK_NATIVE
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
	if (SentryOptions::get_singleton()->is_attach_log_enabled()) {
		String log_path = ProjectSettings::get_singleton()->get_setting("debug/file_logging/log_path");
		if (FileAccess::file_exists(log_path)) {
			log_path = log_path.replace("user://", OS::get_singleton()->get_user_data_dir() + "/");
			attachments.append(log_path);
		} else {
			ERR_PRINT("Sentry: Log file not found. Make sure \"debug/file_logging/enable_file_logging\" is turned ON in the Project Settings.");
		}
	}

	// Attach screenshot.
	if (SentryOptions::get_singleton()->is_attach_screenshot_enabled()) {
		String screenshot_path = OS::get_singleton()->get_user_data_dir().path_join(SENTRY_SCREENSHOT_FN);
		DirAccess::remove_absolute(screenshot_path);
		attachments.append(screenshot_path);
	}

	// Attach view hierarchy (aka scene tree info).
	if (SentryOptions::get_singleton()->is_attach_scene_tree_enabled()) {
		String vh_path = OS::get_singleton()->get_user_data_dir().path_join(SENTRY_VIEW_HIERARCHY_FN);
		DirAccess::remove_absolute(vh_path);
		attachments.append(vh_path);
	}

	return attachments;
}

void SentrySDK::_auto_initialize() {
	sentry::util::print_debug("starting Sentry SDK version " + String(SENTRY_GODOT_SDK_VERSION));

	// Initialize user if it wasn't set explicitly in the configuration script.
	if (user.is_null()) {
		user.instantiate();
		user->set_id(runtime_config->get_installation_id());
		if (SentryOptions::get_singleton()->is_send_default_pii_enabled()) {
			user->infer_ip_address();
		}
	}
	set_user(user);

	bool should_enable = true;

	if (!SentryOptions::get_singleton()->is_enabled()) {
		should_enable = false;
		sentry::util::print_debug("Sentry SDK is disabled in options.");
	}

	if (Engine::get_singleton()->is_editor_hint()) {
		should_enable = false;
		sentry::util::print_debug("Sentry SDK is disabled in the editor.");
	}

	if (!Engine::get_singleton()->is_editor_hint() && OS::get_singleton()->has_feature("editor") && SentryOptions::get_singleton()->is_disabled_in_editor_play()) {
		should_enable = false;
		sentry::util::print_debug("Sentry SDK is disabled when project is played from the editor. Tip: This can be changed in the project settings.");
	}

#if SDK_ANDROID
	if (should_enable) {
		if (OS::get_singleton()->has_feature("editor")) {
			should_enable = false;
		}
	}
#endif

	if (!should_enable) {
		sentry::util::print_info("Sentry SDK is DISABLED! Operations with Sentry SDK will result in no-ops.");
		return;
	}

	init();
}

void SentrySDK::_check_if_configuration_succeeded() {
	if (!configuration_succeeded) {
		// Push error and initialize anyway.
		ERR_PRINT("Sentry: Configuration via user script failed. Will try to initialize SDK anyway.");
		sentry::util::print_error("initializing late because configuration via user script failed");
		_auto_initialize();
	}
}

void SentrySDK::_demo_helper_crash_app() {
	char *ptr = (char *)1;
	sentry::util::print_fatal("Crash by access violation ", ptr); // this is going to crash the app
}

void SentrySDK::notify_options_configured() {
	sentry::util::print_debug("finished configuring options via user script");
	configuration_succeeded = true;
	_auto_initialize();
}

void SentrySDK::prepare_and_auto_initialize() {
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

	// Add event processors.
	if (SentryOptions::get_singleton()->is_attach_screenshot_enabled()) {
		SentryOptions::get_singleton()->add_event_processor(memnew(ScreenshotProcessor));
	}
	if (SentryOptions::get_singleton()->is_attach_scene_tree_enabled()) {
		SentryOptions::get_singleton()->add_event_processor(memnew(ViewHierarchyProcessor));
	}

	// Auto-initialize SDK.
	if (SentryOptions::get_singleton()->get_configuration_script().is_empty() || Engine::get_singleton()->is_editor_hint()) {
		// Early initialization path.
		_auto_initialize();
	} else {
		// Register an autoload singleton, which is a user script extending the
		// `SentryConfiguration` class. It will be instantiated and added to the
		// scene tree by the engine shortly after ScriptServer is initialized.
		// When this happens, the `SentryConfiguration` instance receives
		// `NOTIFICATION_READY`, triggering our notification processing code in
		// C++, which calls `_configure()` on the user script and then invokes
		// `notify_options_configured()` in `SentrySDK`. This, in turn,
		// auto-initializes the SDK.
		sentry::util::print_debug("waiting for user configuration autoload");
		ERR_FAIL_NULL(ProjectSettings::get_singleton());
		ProjectSettings::get_singleton()->set_setting("autoload/SentryConfigurationScript",
				SentryOptions::get_singleton()->get_configuration_script());
		// Ensure issues with the configuration script are detected.
		callable_mp(this, &SentrySDK::_check_if_configuration_succeeded).call_deferred();
	}
}

void SentrySDK::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PREDELETE: {
			if (logger.is_valid()) {
				OS::get_singleton()->remove_logger(logger);
				logger.unref();
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

	ClassDB::bind_method(D_METHOD("is_enabled"), &SentrySDK::is_enabled);
	ClassDB::bind_method(D_METHOD("capture_message", "message", "level"), &SentrySDK::capture_message, DEFVAL(LEVEL_INFO));
	ClassDB::bind_method(D_METHOD("add_breadcrumb", "message", "category", "level", "type", "data"), &SentrySDK::add_breadcrumb, DEFVAL(LEVEL_INFO), DEFVAL("default"), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_last_event_id"), &SentrySDK::get_last_event_id);
	ClassDB::bind_method(D_METHOD("set_context", "key", "value"), &SentrySDK::set_context);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &SentrySDK::set_tag);
	ClassDB::bind_method(D_METHOD("remove_tag", "key"), &SentrySDK::remove_tag);
	ClassDB::bind_method(D_METHOD("set_user", "user"), &SentrySDK::set_user);
	ClassDB::bind_method(D_METHOD("get_user"), &SentrySDK::get_user);
	ClassDB::bind_method(D_METHOD("remove_user"), &SentrySDK::remove_user);
	ClassDB::bind_method(D_METHOD("create_event"), &SentrySDK::create_event);
	ClassDB::bind_method(D_METHOD("capture_event", "event"), &SentrySDK::capture_event);
	ClassDB::bind_method(D_METHOD("add_attachment", "attachment"), &SentrySDK::add_attachment);

	// Hidden API methods -- used in testing.
	ClassDB::bind_method(D_METHOD("_set_before_send", "callable"), &SentrySDK::set_before_send);
	ClassDB::bind_method(D_METHOD("_unset_before_send"), &SentrySDK::unset_before_send);
	ClassDB::bind_method(D_METHOD("_get_before_send"), &SentrySDK::get_before_send);
	ClassDB::bind_method(D_METHOD("_demo_helper_crash_app"), &SentrySDK::_demo_helper_crash_app);
}

SentrySDK::SentrySDK() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(SentryOptions::get_singleton());

	user_mutex.instantiate();

#ifdef SDK_NATIVE
	internal_sdk = std::make_shared<NativeSDK>();
#elif SDK_ANDROID
	if (unlikely(OS::get_singleton()->has_feature("editor"))) {
		sentry::util::print_debug("Sentry SDK is disabled in Android editor mode (only supported in exported Android projects)");
		internal_sdk = std::make_shared<DisabledSDK>();
	} else {
		auto sdk = std::make_shared<AndroidSDK>();
		if (sdk->has_android_plugin()) {
			internal_sdk = sdk;
		} else {
			sentry::util::print_error("Failed to initialize on Android. Disabling Sentry SDK...");
			internal_sdk = std::make_shared<DisabledSDK>();
		}
	}
#elif SDK_COCOA
	internal_sdk = std::make_shared<sentry::cocoa::CocoaSDK>();
#else
	// Unsupported platform
	sentry::util::print_debug("This is an unsupported platform. Disabling Sentry SDK...");
	internal_sdk = std::make_shared<DisabledSDK>();
#endif
}

SentrySDK::~SentrySDK() {
	singleton = nullptr;
}

} // namespace sentry
