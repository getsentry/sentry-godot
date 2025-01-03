#include "sentry_sdk.h"

#include "sdk_version.gen.h"
#include "sentry/contexts.h"
#include "sentry/disabled_sdk.h"
#include "sentry/util.h"
#include "sentry/uuid.h"
#include "sentry_options.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#ifdef NATIVE_SDK
#include "sentry/native/native_sdk.h"
#endif

using namespace godot;
using namespace sentry;

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

Ref<SentryEvent> SentrySDK::create_message_event(const String &p_message, sentry::Level p_level, const String &p_logger) {
	return internal_sdk->create_message_event(p_message, p_level, p_logger);
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
	ERR_FAIL_NULL_MSG(p_user, "Sentry: Setting user failed - user object is null. Please, use Sentry.remove_user() to clear user info.");

	// Initialize user ID if not supplied.
	if (p_user->get_id().is_empty()) {
		if (get_user()->get_id().is_empty() && SentryOptions::get_singleton()->is_send_default_pii_enabled()) {
			p_user->generate_new_id();
		}
	}

	// Save user in a runtime conf-file.
	// TODO: Make saving optional?
	runtime_config->set_user(p_user);
	internal_sdk->set_user(p_user);
}

void SentrySDK::remove_user() {
	internal_sdk->remove_user();
	runtime_config->set_user(memnew(SentryUser));
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
	ClassDB::bind_method(D_METHOD("create_event"), &SentrySDK::create_event);
	ClassDB::bind_method(D_METHOD("create_message_event", "message", "level", "logger"), &SentrySDK::create_message_event, DEFVAL(LEVEL_INFO), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("capture_event", "event"), &SentrySDK::capture_event);
}

SentrySDK::SentrySDK() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(SentryOptions::get_singleton());

	singleton = this;

	sentry::util::print_debug("starting Sentry SDK version " + String(SENTRY_GODOT_SDK_VERSION));

	// Load the runtime configuration from the user's data directory.
	runtime_config.instantiate();
	runtime_config->load_file(OS::get_singleton()->get_user_data_dir() + "/sentry.dat");

#ifdef NATIVE_SDK
	internal_sdk = std::make_shared<NativeSDK>();
	enabled = true;
#else
	// Unsupported platform
	sentry::util::print_debug("This is an unsupported platform. Operations with Sentry SDK will result in no-ops.");
	internal_sdk = std::make_shared<DisabledSDK>();
	return;
#endif

	enabled = enabled && SentryOptions::get_singleton()->is_enabled();
	if (enabled && Engine::get_singleton()->is_editor_hint() && SentryOptions::get_singleton()->is_disabled_in_editor()) {
		sentry::util::print_debug("Sentry SDK is disabled in the editor. Tip: This can be changed in the project settings.");
		enabled = false;
	}
	if (!enabled) {
		sentry::util::print_debug("Sentry SDK is DISABLED! Operations with Sentry SDK will result in no-ops.");
		internal_sdk = std::make_shared<DisabledSDK>();
		return;
	}

	internal_sdk->initialize();

	// Delay the contexts initialization until the engine singletons are ready.
	callable_mp(this, &SentrySDK::_init_contexts).call_deferred();

	// Initialize user.
	set_user(runtime_config->get_user());
}

SentrySDK::~SentrySDK() {
	singleton = nullptr;
}
