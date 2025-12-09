#ifndef SENTRY_OPTIONS_H
#define SENTRY_OPTIONS_H

#include "sentry/godot_error_types.h"
#include "sentry/level.h"
#include "sentry/processing/sentry_event_processor.h"
#include "sentry/util/simple_bind.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry {

// Godot logger integration limits.
class SentryLoggerLimits : public RefCounted {
	GDCLASS(SentryLoggerLimits, RefCounted);

public:
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

// Experimental options.
class SentryExperimental : public RefCounted {
	GDCLASS(SentryExperimental, RefCounted);

public:
	void set_enable_logs(bool p_value);
	bool get_enable_logs();

	void set_before_send_log(Callable p_value);
	Callable get_before_send_log();

protected:
	static void _bind_methods();
};

// Main Sentry options.
class SentryOptions : public RefCounted {
	GDCLASS(SentryOptions, RefCounted);

public:
	using GodotErrorType = sentry::GodotErrorType;
	using GodotErrorMask = sentry::GodotErrorMask;

private:
	static Ref<SentryOptions> singleton;

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
	String environment;
	double sample_rate = 1.0;
	int max_breadcrumbs = 100;
	bool send_default_pii = false;

	bool attach_log = true;
	bool attach_screenshot = false;
	sentry::Level screenshot_level = sentry::LEVEL_FATAL;
	bool attach_scene_tree = false;

	bool enable_logs = true;
	Callable before_send_log;

	bool app_hang_tracking = false;
	double app_hang_timeout_sec = 5.0;

	bool logger_enabled = true;
	bool logger_include_source = true;
	bool logger_include_variables = false;
	bool logger_messages_as_breadcrumbs = true;
	BitField<GodotErrorMask> logger_event_mask = int(GodotErrorMask::MASK_ALL_EXCEPT_WARNING);
	BitField<GodotErrorMask> logger_breadcrumb_mask = int(GodotErrorMask::MASK_ALL);
	Ref<SentryLoggerLimits> logger_limits;

	Ref<SentryExperimental> experimental;

	Callable before_send;
	Callable before_capture_screenshot;

	Vector<Ref<SentryEventProcessor>> event_processors;

	static void _define_project_settings(const Ref<SentryOptions> &p_options);
	static void _load_project_settings(const Ref<SentryOptions> &p_options);

	void _init_debug_option(DebugMode p_debug_mode);

protected:
	static void _bind_methods();

public:
	static void create_singleton();
	static void destroy_singleton();
	_FORCE_INLINE_ static Ref<SentryOptions> get_singleton() { return singleton; }

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
	_FORCE_INLINE_ void set_environment(const String &p_environment) { environment = p_environment; }

	_FORCE_INLINE_ bool is_debug_enabled() const { return debug; }
	_FORCE_INLINE_ void set_debug_enabled(bool p_enabled) { debug = p_enabled; }

	_FORCE_INLINE_ void set_diagnostic_level(sentry::Level p_level) { diagnostic_level = p_level; }
	_FORCE_INLINE_ sentry::Level get_diagnostic_level() const { return diagnostic_level; }

	_FORCE_INLINE_ double get_sample_rate() const { return sample_rate; }
	_FORCE_INLINE_ void set_sample_rate(double p_sample_rate) { sample_rate = p_sample_rate; }

	_FORCE_INLINE_ int get_max_breadcrumbs() const { return max_breadcrumbs; }
	_FORCE_INLINE_ void set_max_breadcrumbs(int p_max_breadcrumbs) { max_breadcrumbs = p_max_breadcrumbs; }

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

	_FORCE_INLINE_ bool is_app_hang_tracking_enabled() const { return app_hang_tracking; }
	_FORCE_INLINE_ void set_app_hang_tracking(bool p_enabled) { app_hang_tracking = p_enabled; }

	_FORCE_INLINE_ double get_app_hang_timeout_sec() const { return app_hang_timeout_sec; }
	_FORCE_INLINE_ void set_app_hang_timeout_sec(double p_seconds) { app_hang_timeout_sec = p_seconds; }

	_FORCE_INLINE_ bool is_logger_enabled() const { return logger_enabled; }
	_FORCE_INLINE_ void set_logger_enabled(bool p_enabled) { logger_enabled = p_enabled; }

	_FORCE_INLINE_ bool is_logger_include_source_enabled() const { return logger_include_source; }
	_FORCE_INLINE_ void set_logger_include_source(bool p_enable) { logger_include_source = p_enable; }

	_FORCE_INLINE_ bool is_logger_include_variables_enabled() const { return logger_include_variables; }
	_FORCE_INLINE_ void set_logger_include_variables(bool p_logger_include_variables) { logger_include_variables = p_logger_include_variables; }

	_FORCE_INLINE_ bool is_logger_messages_as_breadcrumbs_enabled() const { return logger_messages_as_breadcrumbs; }
	_FORCE_INLINE_ void set_logger_messages_as_breadcrumbs(bool p_enabled) { logger_messages_as_breadcrumbs = p_enabled; }

	_FORCE_INLINE_ BitField<GodotErrorMask> get_logger_event_mask() const { return logger_event_mask; }
	_FORCE_INLINE_ void set_logger_event_mask(BitField<GodotErrorMask> p_mask) { logger_event_mask = p_mask; }

	_FORCE_INLINE_ BitField<GodotErrorMask> get_logger_breadcrumb_mask() const { return logger_breadcrumb_mask; }
	_FORCE_INLINE_ void set_logger_breadcrumb_mask(BitField<GodotErrorMask> p_mask) { logger_breadcrumb_mask = p_mask; }

	_FORCE_INLINE_ bool should_capture_event(GodotErrorType p_error_type) { return logger_event_mask.has_flag(sentry::godot_error_type_as_mask(p_error_type)); }
	_FORCE_INLINE_ bool should_capture_breadcrumb(GodotErrorType p_error_type) { return logger_breadcrumb_mask.has_flag(sentry::godot_error_type_as_mask(p_error_type)); }

	_FORCE_INLINE_ Ref<SentryLoggerLimits> get_logger_limits() const { return logger_limits; }
	void set_logger_limits(const Ref<SentryLoggerLimits> &p_limits);

	_FORCE_INLINE_ Callable get_before_send() const { return before_send; }
	_FORCE_INLINE_ void set_before_send(const Callable &p_before_send) { before_send = p_before_send; }

	_FORCE_INLINE_ Callable get_before_capture_screenshot() const { return before_capture_screenshot; }
	_FORCE_INLINE_ void set_before_capture_screenshot(const Callable &p_before_capture_screenshot) { before_capture_screenshot = p_before_capture_screenshot; }

	_FORCE_INLINE_ Ref<SentryExperimental> get_experimental() const { return experimental; }

	void add_event_processor(const Ref<SentryEventProcessor> &p_processor);
	void remove_event_processor(const Ref<SentryEventProcessor> &p_processor);
	_FORCE_INLINE_ Vector<Ref<SentryEventProcessor>> get_event_processors() { return event_processors; }

	SentryOptions();
	~SentryOptions();
};

} // namespace sentry

VARIANT_BITFIELD_CAST(sentry::SentryOptions::GodotErrorMask);

#endif // SENTRY_OPTIONS_H
