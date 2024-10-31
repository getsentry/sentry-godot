#ifndef SENTRY_SINGLETON_H
#define SENTRY_SINGLETON_H

#include "runtime_config.h"
#include "sentry/internal_sdk.h"
#include "sentry/level.h"
#include "sentry_user.h"

// #include <sentry.h>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <memory>

using namespace godot;

// Entry point for Sentry SDK users.
// This singleton class exposes the public API of the Sentry SDK for the Godot Engine.
class SentrySDK : public Object {
	GDCLASS(SentrySDK, Object);

private:
	static SentrySDK *singleton;

	std::shared_ptr<sentry::InternalSDK> internal_sdk;

	RuntimeConfig runtime_config;

	// TODO: Fix performance context!
	// sentry_value_t _create_performance_context();

protected:
	static void _bind_methods();

public:
	static SentrySDK *get_singleton() { return singleton; }

	// TODO: Fix hooks!
	// Called by internal SDKs to process events before sending them.
	// sentry_value_t handle_before_send(sentry_value_t p_event);
	// sentry_value_t handle_on_crash(sentry_value_t p_event);

	// TODO: Move out of SentrySDK.
	void add_device_context();
	void add_app_context();
	void add_gpu_context();
	void add_culture_context();
	void add_display_context();
	void add_engine_context();
	void add_environment_context();

	CharString get_environment() const;

	void add_breadcrumb(const String &p_message, const String &p_category, sentry::Level p_level,
			const String &p_type = "default", const Dictionary &p_data = Dictionary());
	void set_context(const String &p_key, const Dictionary &p_value);

	void set_tag(const String &p_key, const String &p_value);
	void remove_tag(const String &p_key);

	void set_user(const Ref<SentryUser> &p_user);
	Ref<SentryUser> get_user() const { return runtime_config.get_user(); }
	void remove_user();

	void capture_message(const String &p_message, sentry::Level p_level, const String &p_logger = "");
	String get_last_event_id() const;

	SentrySDK();
	~SentrySDK();
};

#endif // SENTRY_SINGLETON_H