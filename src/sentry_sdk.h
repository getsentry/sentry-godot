#ifndef SENTRY_SINGLETON_H
#define SENTRY_SINGLETON_H

#include "runtime_config.h"
#include "sentry/internal_sdk.h"
#include "sentry/level.h"
#include "sentry_event.h"

#include <godot_cpp/core/object.hpp>
#include <memory>

using namespace godot;

// Entry point for Sentry SDK users.
// This singleton class exposes the public API of the Sentry SDK for the Godot Engine.
class SentrySDK : public Object {
	GDCLASS(SentrySDK, Object);

public:
	// SentrySDK.Level is actually defined in sentry/level.h.
	// In Godot API, extensions can't expose global enums - it must belong to a class.
	// This structure is needed to avoid circular dependencies between this and other headers that use Level enum.
	using Level = sentry::Level;

private:
	static SentrySDK *singleton;

	std::shared_ptr<sentry::InternalSDK> internal_sdk;
	Ref<RuntimeConfig> runtime_config;
	bool enabled = false;

	void _init_contexts();

protected:
	static void _bind_methods();

public:
	static SentrySDK *get_singleton() { return singleton; }

	_FORCE_INLINE_ std::shared_ptr<sentry::InternalSDK> get_internal_sdk() const { return internal_sdk; }

	// * Exported API

	bool is_enabled() const { return enabled; }

	void add_breadcrumb(const String &p_message, const String &p_category, sentry::Level p_level,
			const String &p_type = "default", const Dictionary &p_data = Dictionary());
	void set_context(const String &p_key, const Dictionary &p_value);

	void set_tag(const String &p_key, const String &p_value);
	void remove_tag(const String &p_key);

	void set_user(const Ref<SentryUser> &p_user);
	Ref<SentryUser> get_user() const { return runtime_config->get_user(); }
	void remove_user();

	String capture_message(const String &p_message, sentry::Level p_level = sentry::LEVEL_INFO, const String &p_logger = "");
	String get_last_event_id() const;

	Ref<SentryEvent> create_event() const;
	String capture_event(const Ref<SentryEvent> &p_event);

	SentrySDK();
	~SentrySDK();
};

VARIANT_ENUM_CAST(SentrySDK::Level);

#endif // SENTRY_SINGLETON_H
