#pragma once

#include "sentry/godot_error_types.h"
#include "sentry/level.h"
#include "sentry/processing/sentry_event_processor.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_scope_observer.h"
#include "sentry/util/simple_bind.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry {

// Godot logger integration limits.
class SentryLoggerLimits : public RefCounted {
	GDCLASS(SentryLoggerLimits, RefCounted);

	// Protect frametime budget.
	SIMPLE_PROPERTY(int, events_per_frame, 5);

	// Limit to 1 error captured per source line within T milliseconds window.
	SIMPLE_PROPERTY(int, repeated_error_window_ms, 1000);

	// Limit to N events within T milliseconds window.
	SIMPLE_PROPERTY(int, throttle_events, 20);
	SIMPLE_PROPERTY(int, throttle_window_ms, 10000);

protected:
	static void _bind_methods();
};

// Godot logger integration options.
class SentryGodotLoggerOptions : public RefCounted {
	GDCLASS(SentryGodotLoggerOptions, RefCounted);

	SIMPLE_PROPERTY(bool, enabled, true);
	SIMPLE_PROPERTY(bool, include_source_context, true);
	SIMPLE_PROPERTY(bool, include_variables, false);
	SIMPLE_PROPERTY(BitField<GodotLoggerEventMask>, event_mask, GodotLoggerEventMask::MASK_ERROR | GodotLoggerEventMask::MASK_SCRIPT | GodotLoggerEventMask::MASK_SHADER);
	SIMPLE_PROPERTY(BitField<GodotLoggerEventMask>, breadcrumb_mask, GodotLoggerEventMask::MASK_ALL);
	SIMPLE_PROPERTY(BitField<GodotLoggerEventMask>, log_mask, GodotLoggerEventMask::MASK_NONE);

private:
	Ref<SentryLoggerLimits> limits;

protected:
	static void _bind_methods();

public:
	_FORCE_INLINE_ Ref<SentryLoggerLimits> get_limits() const { return limits; }
	void deprecated_set_limits(const Ref<SentryLoggerLimits> &p_limits);

	SentryGodotLoggerOptions();
};

class SentryOptions;

// Experimental options.
class SentryExperimental : public RefCounted {
	GDCLASS(SentryExperimental, RefCounted);

	// Keep owner and the friend wiring for future compatibility forwarders.
	friend class SentryOptions;
	SentryOptions *owner = nullptr;

	bool enable_metrics = true;
	Callable before_send_metric;

public:
	void set_enable_metrics(bool p_value) { enable_metrics = p_value; }
	bool get_enable_metrics() const { return enable_metrics; }

	void set_before_send_metric(const Callable &p_value) { before_send_metric = p_value; }
	Callable get_before_send_metric() const { return before_send_metric; }

protected:
	static void _bind_methods();
};

class SentryAndroidOptions : public RefCounted {
	GDCLASS(SentryAndroidOptions, RefCounted);

	SIMPLE_PROPERTY(bool, enable_anr_detection, true);
	SIMPLE_PROPERTY(int, anr_timeout_interval_ms, 5000);
	SIMPLE_PROPERTY(bool, attach_anr_thread_dump, false);

protected:
	static void _bind_methods();
};

// Main Sentry options.
class SentryOptions : public RefCounted {
	GDCLASS(SentryOptions, RefCounted);

public:
	using GodotErrorType = sentry::GodotErrorType;
	using GodotLoggerEventMask = sentry::GodotLoggerEventMask;

private:
	enum class DebugMode {
		DEBUG_OFF = 0,
		DEBUG_ON = 1,
		DEBUG_AUTO = 2,
	};
	static constexpr DebugMode DEBUG_DEFAULT = DebugMode::DEBUG_AUTO;

	bool auto_init = true;
	bool skip_auto_init_on_editor_play = false;
	String dsn = "";
	String release = "{app_name}@{app_version}";
	String dist = "";
	bool debug = false;
	sentry::Level diagnostic_level = sentry::LEVEL_DEBUG;
	String environment = "{auto}";
	double sample_rate = 1.0;
	int max_breadcrumbs = 100;
	int shutdown_timeout_ms = 2000;
	bool send_default_pii = false;

	bool attach_log = true;
	bool attach_screenshot = false;
	sentry::Level screenshot_level = sentry::LEVEL_FATAL;
	bool attach_scene_tree = false;

	bool enable_logs = true;
	Callable before_send_log;

	bool enable_app_hang_tracking = false;
	int app_hang_timeout_ms = 5000;

	Ref<SentryExperimental> experimental;
	Ref<SentryAndroidOptions> android;
	Ref<SentryGodotLoggerOptions> godot_logger;

	Callable before_send;
	Callable before_capture_screenshot;

	Vector<Ref<SentryEventProcessor>> event_processors;
	Vector<Ref<SentryScopeObserver>> scope_observers;
	// Default attachments (log, screenshot, view hierarchy). Must be file-based. Survive clear_attachments().
	Vector<Ref<SentryAttachment>> default_attachments;
	// User attachments added during config callback, drained at init.
	Vector<Ref<SentryAttachment>> custom_attachments;

	static void _define_project_settings(const Ref<SentryOptions> &p_options);
	static void _load_project_settings(const Ref<SentryOptions> &p_options);

	void _init_debug_option(DebugMode p_debug_mode);

protected:
	static void _bind_methods();

public:
	static Ref<SentryOptions> create_from_project_settings();

	_FORCE_INLINE_ bool is_auto_init_enabled() const { return auto_init; }
	_FORCE_INLINE_ void set_auto_init(bool p_enabled) { auto_init = p_enabled; }

	_FORCE_INLINE_ bool should_skip_auto_init_on_editor_play() const { return skip_auto_init_on_editor_play; }
	_FORCE_INLINE_ void set_skip_auto_init_on_editor_play(bool p_skip) { skip_auto_init_on_editor_play = p_skip; }

	_FORCE_INLINE_ String get_dsn() const { return dsn; }
	_FORCE_INLINE_ void set_dsn(const String &p_dsn) { dsn = p_dsn; }

	_FORCE_INLINE_ String get_release() const { return release; }
	void set_release(const String &p_release);

	_FORCE_INLINE_ void set_dist(const String &p_dist) { dist = p_dist; }
	_FORCE_INLINE_ String get_dist() const { return dist; }

	_FORCE_INLINE_ String get_environment() const { return environment; }
	void set_environment(const String &p_environment);

	_FORCE_INLINE_ bool is_debug_enabled() const { return debug; }
	_FORCE_INLINE_ void set_debug_enabled(bool p_enabled) { debug = p_enabled; }

	_FORCE_INLINE_ void set_diagnostic_level(sentry::Level p_level) { diagnostic_level = p_level; }
	_FORCE_INLINE_ sentry::Level get_diagnostic_level() const { return diagnostic_level; }

	_FORCE_INLINE_ double get_sample_rate() const { return sample_rate; }
	_FORCE_INLINE_ void set_sample_rate(double p_sample_rate) { sample_rate = p_sample_rate; }

	_FORCE_INLINE_ int get_max_breadcrumbs() const { return max_breadcrumbs; }
	_FORCE_INLINE_ void set_max_breadcrumbs(int p_max_breadcrumbs) { max_breadcrumbs = p_max_breadcrumbs; }

	_FORCE_INLINE_ int get_shutdown_timeout_ms() const { return shutdown_timeout_ms; }
	_FORCE_INLINE_ void set_shutdown_timeout_ms(int p_shutdown_timeout_ms) { shutdown_timeout_ms = p_shutdown_timeout_ms; }

	_FORCE_INLINE_ bool is_send_default_pii_enabled() const { return send_default_pii; }
	_FORCE_INLINE_ void set_send_default_pii(bool p_enabled) { send_default_pii = p_enabled; }

	_FORCE_INLINE_ bool is_attach_log_enabled() const { return attach_log; }
	_FORCE_INLINE_ void set_attach_log(bool p_enabled) { attach_log = p_enabled; }

	_FORCE_INLINE_ bool is_attach_screenshot_enabled() const { return attach_screenshot; }
	_FORCE_INLINE_ void set_attach_screenshot(bool p_attach_screenshot) { attach_screenshot = p_attach_screenshot; }

	_FORCE_INLINE_ sentry::Level get_screenshot_level() const { return screenshot_level; }
	_FORCE_INLINE_ void set_screenshot_level(sentry::Level p_level) { screenshot_level = p_level; }

	_FORCE_INLINE_ void set_attach_scene_tree(bool p_enable) { attach_scene_tree = p_enable; }
	_FORCE_INLINE_ bool is_attach_scene_tree_enabled() const { return attach_scene_tree; }

	_FORCE_INLINE_ bool get_enable_logs() const { return enable_logs; }
	_FORCE_INLINE_ void set_enable_logs(bool p_enabled) { enable_logs = p_enabled; }

	_FORCE_INLINE_ Callable get_before_send_log() const { return before_send_log; }
	_FORCE_INLINE_ void set_before_send_log(const Callable &p_callback) { before_send_log = p_callback; }

	_FORCE_INLINE_ bool is_app_hang_tracking_enabled() const { return enable_app_hang_tracking; }
	_FORCE_INLINE_ void set_app_hang_tracking_enabled(bool p_enabled) { enable_app_hang_tracking = p_enabled; }

	_FORCE_INLINE_ int get_app_hang_timeout_ms() const { return app_hang_timeout_ms; }
	_FORCE_INLINE_ void set_app_hang_timeout_ms(int p_milliseconds) { app_hang_timeout_ms = p_milliseconds; }

	_FORCE_INLINE_ Callable get_before_send() const { return before_send; }
	_FORCE_INLINE_ void set_before_send(const Callable &p_before_send) { before_send = p_before_send; }

	_FORCE_INLINE_ Callable get_before_capture_screenshot() const { return before_capture_screenshot; }
	_FORCE_INLINE_ void set_before_capture_screenshot(const Callable &p_before_capture_screenshot) { before_capture_screenshot = p_before_capture_screenshot; }

	_FORCE_INLINE_ Ref<SentryExperimental> get_experimental() const { return experimental; }
	_FORCE_INLINE_ Ref<SentryAndroidOptions> get_android() const { return android; }
	_FORCE_INLINE_ Ref<SentryGodotLoggerOptions> get_godot_logger() const { return godot_logger; }

	_FORCE_INLINE_ bool should_capture_event(GodotErrorType p_error_type) { return godot_logger->get_event_mask().has_flag(sentry::godot_error_type_as_mask(p_error_type)); }
	_FORCE_INLINE_ bool should_capture_breadcrumb(GodotErrorType p_error_type) { return godot_logger->get_breadcrumb_mask().has_flag(sentry::godot_error_type_as_mask(p_error_type)); }
	_FORCE_INLINE_ bool should_capture_log(GodotErrorType p_error_type) { return enable_logs && godot_logger->get_log_mask().has_flag(sentry::godot_error_type_as_mask(p_error_type)); }
	_FORCE_INLINE_ bool should_capture_message_breadcrumb() { return godot_logger->get_breadcrumb_mask().has_flag(MASK_MESSAGE); }
	_FORCE_INLINE_ bool should_capture_message_log() { return enable_logs && godot_logger->get_log_mask().has_flag(MASK_MESSAGE); }

	void add_event_processor(const Ref<SentryEventProcessor> &p_processor);
	void remove_event_processor(const Ref<SentryEventProcessor> &p_processor);
	_FORCE_INLINE_ Vector<Ref<SentryEventProcessor>> get_event_processors() { return event_processors; }

	void add_scope_observer(const Ref<SentryScopeObserver> &p_scope_observer);
	_FORCE_INLINE_ Vector<Ref<SentryScopeObserver>> get_scope_observers() { return scope_observers; }

	void add_default_attachment(const Ref<SentryAttachment> &p_attachment);
	_FORCE_INLINE_ Vector<Ref<SentryAttachment>> get_default_attachments() const { return default_attachments; }

	void add_custom_attachment(const Ref<SentryAttachment> &p_attachment) { custom_attachments.append(p_attachment); }
	_FORCE_INLINE_ Vector<Ref<SentryAttachment>> get_custom_attachments() const { return custom_attachments; }
	void clear_custom_attachments() { custom_attachments.clear(); }

	SentryOptions();
	~SentryOptions();

	// *** Deprecated

	bool deprecated_get_app_hang_tracking() const;
	void deprecated_set_app_hang_tracking(bool p_enabled);

	double deprecated_get_app_hang_timeout_sec() const;
	void deprecated_set_app_hang_timeout_sec(double p_seconds);

	bool deprecated_is_logger_enabled() const { return godot_logger->get_enabled(); }
	void deprecated_set_logger_enabled(bool p_enabled);

	bool deprecated_is_logger_include_source_enabled() const { return godot_logger->get_include_source_context(); }
	void deprecated_set_logger_include_source(bool p_enable);

	bool deprecated_is_logger_include_variables_enabled() const { return godot_logger->get_include_variables(); }
	void deprecated_set_logger_include_variables(bool p_logger_include_variables);

	bool deprecated_is_logger_messages_as_breadcrumbs_enabled() const { return godot_logger->get_breadcrumb_mask().has_flag(GodotLoggerEventMask::MASK_MESSAGE); }
	void deprecated_set_logger_messages_as_breadcrumbs(bool p_enabled);

	BitField<GodotLoggerEventMask> deprecated_get_logger_event_mask() const { return godot_logger->get_event_mask(); }
	void deprecated_set_logger_event_mask(BitField<GodotLoggerEventMask> p_mask);

	BitField<GodotLoggerEventMask> deprecated_get_logger_breadcrumb_mask() const { return godot_logger->get_breadcrumb_mask(); }
	void deprecated_set_logger_breadcrumb_mask(BitField<GodotLoggerEventMask> p_mask);

	BitField<GodotLoggerEventMask> deprecated_get_logger_log_mask() const { return godot_logger->get_log_mask(); }
	void deprecated_set_logger_log_mask(BitField<GodotLoggerEventMask> p_mask);

	Ref<SentryLoggerLimits> deprecated_get_logger_limits() const { return godot_logger->get_limits(); }
	void deprecated_set_logger_limits(const Ref<SentryLoggerLimits> &p_limits);
};

} // namespace sentry

VARIANT_BITFIELD_CAST(sentry::SentryOptions::GodotLoggerEventMask);
