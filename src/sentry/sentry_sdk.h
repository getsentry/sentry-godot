#ifndef SENTRY_SINGLETON_H
#define SENTRY_SINGLETON_H

#include "runtime_config.h"
#include "sentry/internal_sdk.h"
#include "sentry/level.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_event.h"
#include "sentry/sentry_logger.h"
#include "sentry/sentry_options.h"

#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/core/object.hpp>
#include <memory>

using namespace godot;

namespace sentry {

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
	Ref<SentryLogger> logger;
	bool is_auto_initializing = false;

	void _init_contexts();
	void _init_user();
	PackedStringArray _get_global_attachments();
	void _auto_initialize();
	void _demo_helper_crash_app();

protected:
	static void _bind_methods();

	void _notification(int p_what);

public:
	static void create_singleton();
	static void destroy_singleton();
	static SentrySDK *get_singleton() { return singleton; }

	_FORCE_INLINE_ std::shared_ptr<sentry::InternalSDK> get_internal_sdk() const { return internal_sdk; }
	_FORCE_INLINE_ Ref<RuntimeConfig> get_runtime_config() const { return runtime_config; }

	// * Exported API

	void init(const Callable &p_configuration_callback = Callable());
	void close();
	bool is_enabled() const { return internal_sdk->is_enabled(); }

	void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb);

	void set_context(const String &p_key, const Dictionary &p_value);

	void set_tag(const String &p_key, const String &p_value);
	void remove_tag(const String &p_key);

	void set_user(const Ref<SentryUser> &p_user);
	void remove_user();

	String capture_message(const String &p_message, sentry::Level p_level = sentry::LEVEL_INFO);
	String get_last_event_id() const;

	Ref<SentryEvent> create_event() const;
	String capture_event(const Ref<SentryEvent> &p_event);

	void add_attachment(const Ref<SentryAttachment> &p_attachment);

	// * Hidden API methods -- used in testing

	void set_before_send(const Callable &p_callable) { SentryOptions::get_singleton()->set_before_send(p_callable); }
	void unset_before_send() { SentryOptions::get_singleton()->set_before_send(Callable()); }
	Callable get_before_send() { return SentryOptions::get_singleton()->get_before_send(); }

	void prepare_and_auto_initialize();

	SentrySDK();
	~SentrySDK();
};

} // namespace sentry

VARIANT_ENUM_CAST(sentry::SentrySDK::Level);

#endif // SENTRY_SINGLETON_H
