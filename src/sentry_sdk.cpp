#include "sentry_sdk.h"

#include "gen/sdk_version.gen.h"
#include "sentry/contexts.h"
#include "sentry/disabled_sdk.h"
#include "sentry/util.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#ifdef NATIVE_SDK
#include "sentry/native/native_sdk.h"
#endif

using namespace godot;
using namespace sentry;

namespace {

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
		if (err != OK) {
			sentry::util::print_error(vformat("Failed to set executable permissions for %s (%d)", p_path.utf8().get_data(), err));
		}
	}
}

} // unnamed namespace

SentrySDK *SentrySDK::singleton = nullptr;

String SentrySDK::capture_message(const String &p_message, Level p_level, const String &p_logger) {
	return internal_sdk->capture_message(p_message, p_level, p_logger);
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

void SentrySDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set tag with an empty key.");
	internal_sdk->set_tag(p_key, p_value);
}

void SentrySDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't remove tag with an empty key.");
	internal_sdk->remove_tag(p_key);
}

void SentrySDK::set_user(const Ref<SentryUser> &p_user) {
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

void SentrySDK::remove_user() {
	user.instantiate();
	internal_sdk->remove_user();
}

void SentrySDK::set_context(const godot::String &p_key, const godot::Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set context with an empty key.");
	internal_sdk->set_context(p_key, p_value);
}

void SentrySDK::_init_contexts() {
	sentry::util::print_debug("initializing contexts");
	internal_sdk->set_context("device", sentry::contexts::make_device_context(runtime_config));
	internal_sdk->set_context("app", sentry::contexts::make_app_context());
	internal_sdk->set_context("gpu", sentry::contexts::make_gpu_context());
	internal_sdk->set_context("culture", sentry::contexts::make_culture_context());

	// Custom contexts.
	internal_sdk->set_context("display", sentry::contexts::make_display_context());
	internal_sdk->set_context("godot_engine", sentry::contexts::make_godot_engine_context());
	internal_sdk->set_context("environment", sentry::contexts::make_environment_context());
}

void SentrySDK::_initialize() {
	sentry::util::print_debug("starting Sentry SDK version " + String(SENTRY_GODOT_SDK_VERSION));

#ifdef NATIVE_SDK
	internal_sdk = std::make_shared<NativeSDK>();
	enabled = true;
#else
	// Unsupported platform
	sentry::util::print_debug("This is an unsupported platform. Disabling Sentry SDK...");
	enabled = false;
#endif

	if (!enabled) {
		sentry::util::print_debug("Sentry SDK is DISABLED! Operations with Sentry SDK will result in no-ops.");
		internal_sdk = std::make_shared<DisabledSDK>();
		return;
	}

	// Initialize user if it wasn't set explicitly in the configuration script.
	if (user.is_null()) {
		user.instantiate();
		user->set_id(runtime_config->get_installation_id());
		if (SentryOptions::get_singleton()->is_send_default_pii_enabled()) {
			user->infer_ip_address();
		}
	}
	set_user(user);

	internal_sdk->initialize();
}

void SentrySDK::_check_if_configuration_succeeded() {
	if (!configuration_succeeded) {
		// Push error and initialize anyway.
		ERR_PRINT("Sentry: Configuration via user script failed. Will try to initialize SDK anyway.");
		sentry::util::print_error("initializing late because configuration via user script failed");
		_initialize();
		SentrySDK::_init_contexts();
	}
}

void SentrySDK::notify_options_configured() {
	sentry::util::print_debug("finished configuring options via user script");
	configuration_succeeded = true;
	_initialize();
	SentrySDK::_init_contexts();
}

void SentrySDK::_bind_methods() {
	BIND_ENUM_CONSTANT(LEVEL_DEBUG);
	BIND_ENUM_CONSTANT(LEVEL_INFO);
	BIND_ENUM_CONSTANT(LEVEL_WARNING);
	BIND_ENUM_CONSTANT(LEVEL_ERROR);
	BIND_ENUM_CONSTANT(LEVEL_FATAL);

	ClassDB::bind_method(D_METHOD("is_enabled"), &SentrySDK::is_enabled);
	ClassDB::bind_method(D_METHOD("capture_message", "message", "level", "logger"), &SentrySDK::capture_message, DEFVAL(LEVEL_INFO), DEFVAL(""));
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

	// Hidden API methods -- used in testing.
	ClassDB::bind_method(D_METHOD("_set_before_send", "callable"), &SentrySDK::set_before_send);
	ClassDB::bind_method(D_METHOD("_unset_before_send"), &SentrySDK::unset_before_send);
	ClassDB::bind_method(D_METHOD("_set_on_crash", "callable"), &SentrySDK::set_on_crash);
	ClassDB::bind_method(D_METHOD("_unset_on_crash"), &SentrySDK::unset_on_crash);
}

SentrySDK::SentrySDK() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(SentryOptions::get_singleton());

	singleton = this;

	// Load the runtime configuration from the user's data directory.
	runtime_config.instantiate();
	runtime_config->load_file(OS::get_singleton()->get_user_data_dir() + "/sentry.dat");

#if defined(LINUX_ENABLED) || defined(WINDOWS_ENABLED)
	// Fix crashpad handler executable bit permissions on Unix platforms if the
	// user extracts the distribution archive without preserving such permissions.
	if (OS::get_singleton()->is_debug_build()) {
		_fix_unix_executable_permissions("res://addons/sentrysdk/bin/macos/crashpad_handler");
		_fix_unix_executable_permissions("res://addons/sentrysdk/bin/linux/crashpad_handler");
	}
#endif

	bool should_enable = SentryOptions::get_singleton()->is_enabled();

	if (!should_enable) {
		sentry::util::print_debug("Sentry SDK is disabled in the project settings.");
	}

	if (should_enable && Engine::get_singleton()->is_editor_hint() && SentryOptions::get_singleton()->is_disabled_in_editor()) {
		sentry::util::print_debug("Sentry SDK is disabled in the editor. Tip: This can be changed in the project settings.");
		should_enable = false;
	}

	if (should_enable) {
		if (SentryOptions::get_singleton()->get_configuration_script().is_empty() || Engine::get_singleton()->is_editor_hint()) {
			_initialize();
			// Delay contexts initialization until the engine singletons are ready.
			callable_mp(this, &SentrySDK::_init_contexts).call_deferred();
		} else {
			// Register an autoload singleton, which is a user script extending the
			// `SentryConfiguration` class. It will be instantiated and added to the
			// scene tree by the engine shortly after ScriptServer is initialized.
			// When this happens, the `SentryConfiguration` instance receives
			// `NOTIFICATION_READY`, triggering our notification processing code in
			// C++, which calls `_configure()` on the user script and then invokes
			// `notify_options_configured()` in `SentrySDK`. This, in turn, initializes
			// the internal SDK.
			internal_sdk = std::make_shared<DisabledSDK>(); // just in case
			sentry::util::print_debug("waiting for user configuration autoload");
			ERR_FAIL_NULL(ProjectSettings::get_singleton());
			ProjectSettings::get_singleton()->set_setting("autoload/SentryConfigurationScript",
					SentryOptions::get_singleton()->get_configuration_script());
			// Ensure issues with the configuration script are detected.
			callable_mp(this, &SentrySDK::_check_if_configuration_succeeded).call_deferred();
		}
	}
}

SentrySDK::~SentrySDK() {
	singleton = nullptr;
}
