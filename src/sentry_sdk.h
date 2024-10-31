#ifndef SENTRY_SINGLETON_H
#define SENTRY_SINGLETON_H

#include "runtime_config.h"
#include "sentry/internal_sdk.h"
#include "sentry/level.h"
#include "sentry_user.h"

// #include <sentry.h>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/object.hpp>
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

	Ref<RuntimeConfig> runtime_config;

	void _init_contexts();
	// TODO: Fix performance context!
	Dictionary _create_performance_context();

protected:
	static void _bind_methods();

public:
	static SentrySDK *get_singleton() { return singleton; }

	// * Exported API

	void add_breadcrumb(const String &p_message, const String &p_category, sentry::Level p_level,
			const String &p_type = "default", const Dictionary &p_data = Dictionary());
	void set_context(const String &p_key, const Dictionary &p_value);

	void set_tag(const String &p_key, const String &p_value);
	void remove_tag(const String &p_key);

	void set_user(const Ref<SentryUser> &p_user);
	Ref<SentryUser> get_user() const { return runtime_config->get_user(); }
	void remove_user();

	void capture_message(const String &p_message, sentry::Level p_level, const String &p_logger = "");
	String get_last_event_id() const;

	SentrySDK();
	~SentrySDK();
};

#endif // SENTRY_SINGLETON_H
