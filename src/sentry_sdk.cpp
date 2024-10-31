#include "sentry_sdk.h"

#include "sentry/contexts.h"
#include "sentry/environment.h"
#include "sentry/native/native_sdk.h"
#include "sentry/uuid.h"
#include "sentry_options.h"
#include "sentry_util.h"

// #include <sentry.h>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;
using namespace sentry;

SentrySDK *SentrySDK::singleton = nullptr;

VARIANT_ENUM_CAST(Level);

void SentrySDK::capture_message(const String &p_message, Level p_level, const String &p_logger) {
	internal_sdk->capture_message(p_message, p_level, p_logger);
}

void SentrySDK::add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
		const String &p_type, const Dictionary &p_data) {
	internal_sdk->add_breadcrumb(p_message, p_category, p_level, p_type, p_data);
}

String SentrySDK::get_last_event_id() const {
	return internal_sdk->get_last_event_id();
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
	ERR_FAIL_NULL_MSG(p_user, "Sentry: Setting user failed - user object is null. Please, use Sentry.remove_user() to clear user info.");

	// Initialize user ID if not supplied.
	if (p_user->get_id().is_empty()) {
		// Take user ID from the runtime config or generate a new one if it's empty.
		String user_id = get_user()->get_id();
		if (user_id.is_empty()) {
			user_id = sentry::uuid::make_uuid();
		}
		p_user->set_id(user_id);
	}

	// Save user in a runtime conf-file.
	// TODO: Make saving optional?
	runtime_config->set_user(p_user);
	internal_sdk->set_user(p_user);
}

void SentrySDK::remove_user() {
	internal_sdk->remove_user();
}

void SentrySDK::set_context(const godot::String &p_key, const godot::Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set context with an empty key.");
	internal_sdk->set_context(p_key, p_value);
}

// TODO: Fix handlers!
// sentry_value_t SentrySDK::handle_before_send(sentry_value_t p_event) {
// 	SentryUtil::sentry_event_set_context(p_event, "Performance", _create_performance_context());
// 	return p_event;
// }

// sentry_value_t SentrySDK::handle_on_crash(sentry_value_t p_event) {
// 	SentryUtil::sentry_event_set_context(p_event, "Performance", _create_performance_context());
// 	return p_event;
// }

void SentrySDK::_bind_methods() {
	BIND_ENUM_CONSTANT(LEVEL_DEBUG);
	BIND_ENUM_CONSTANT(LEVEL_INFO);
	BIND_ENUM_CONSTANT(LEVEL_WARNING);
	BIND_ENUM_CONSTANT(LEVEL_ERROR);
	BIND_ENUM_CONSTANT(LEVEL_FATAL);

	ClassDB::bind_method(D_METHOD("capture_message", "message", "level", "logger"), &SentrySDK::capture_message, DEFVAL(LEVEL_INFO), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("add_breadcrumb", "message", "category", "level", "type", "data"), &SentrySDK::add_breadcrumb, DEFVAL(LEVEL_INFO), DEFVAL("default"), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_last_event_id"), &SentrySDK::get_last_event_id);
	ClassDB::bind_method(D_METHOD("set_context", "key", "value"), &SentrySDK::set_context);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &SentrySDK::set_tag);
	ClassDB::bind_method(D_METHOD("remove_tag", "key"), &SentrySDK::remove_tag);
	ClassDB::bind_method(D_METHOD("set_user", "user"), &SentrySDK::set_user);
	ClassDB::bind_method(D_METHOD("get_user"), &SentrySDK::get_user);
	ClassDB::bind_method(D_METHOD("remove_user"), &SentrySDK::remove_user);
}

void SentrySDK::_init_contexts() {
	internal_sdk->set_context("device", sentry::contexts::make_device_context(runtime_config));
	internal_sdk->set_context("app", sentry::contexts::make_app_context());
	internal_sdk->set_context("gpu", sentry::contexts::make_gpu_context());
	internal_sdk->set_context("culture", sentry::contexts::make_culture_context());

	// Custom contexts.
	internal_sdk->set_context("Display", sentry::contexts::make_display_context());
	internal_sdk->set_context("Godot Engine", sentry::contexts::make_godot_engine_context());
	internal_sdk->set_context("Environment", sentry::contexts::make_environment_context());
}

SentrySDK::SentrySDK() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(SentryOptions::get_singleton());

	singleton = this;

#if defined(LINUX_ENABLED) || defined(WINDOWS_ENABLED) || defined(MACOS_ENABLED)
	internal_sdk = std::make_shared<NativeSDK>();
#else
	// Unsupported platform
	// TODO: Create fake SDK?
	return;
#endif
	internal_sdk->initialize();

	// Setup logging.

	if (!SentryOptions::get_singleton()->is_enabled()) {
		return;
	}

	// Load the runtime configuration from the user's data directory.
	runtime_config.instantiate();
	runtime_config->load_file(OS::get_singleton()->get_user_data_dir() + "/sentry.dat");

	// Delay the contexts initialization until the engine singletons are ready.
	callable_mp(this, &SentrySDK::_init_contexts).call_deferred();

	// TODO: Fix hooks!
	// "before_send" hook.
	// auto before_send_lambda = [](sentry_value_t event, void *hint, void *closure) {
	// 	return SentrySDK::get_singleton()->handle_before_send(event);
	// };
	// sentry_options_set_before_send(options, before_send_lambda, NULL);

	// "on_crash" hook.
	// auto on_crash_lambda = [](const sentry_ucontext_t *uctx, sentry_value_t event, void *closure) {
	// 	return SentrySDK::get_singleton()->handle_on_crash(event);
	// };
	// sentry_options_set_on_crash(options, on_crash_lambda, NULL);

	// Initialize user.
	set_user(runtime_config->get_user());
}

SentrySDK::~SentrySDK() {
	singleton = nullptr;
}
