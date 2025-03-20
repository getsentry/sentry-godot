#ifndef SENTRY_OPTIONS_H
#define SENTRY_OPTIONS_H

#include "sentry/godot_error_types.h"
#include "sentry/level.h"
#include "sentry/simple_bind.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

class SentryLoggerLimits : public RefCounted {
	GDCLASS(SentryLoggerLimits, RefCounted);

public:
	// Limit the number of lines that can be parsed per frame.
	SIMPLE_PROPERTY(int, parse_lines, 100);

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

	bool enabled = true;
	bool disabled_in_editor = true;
	String dsn = "";
	String release = "{app_name}@{app_version}";
	String dist = "";
	bool debug = false;
	sentry::Level debug_verbosity = sentry::LEVEL_DEBUG;
	String environment;
	double sample_rate = 1.0;
	int max_breadcrumbs = 100;
	bool send_default_pii = false;

	bool attach_log = true;
	bool attach_screenshot = false;
	bool attach_scene_tree_info = false;

	Vector<StringName> scene_tree_extra_properties;

	bool error_logger_enabled = true;
	bool error_logger_include_source = true;
	BitField<GodotErrorMask> error_logger_event_mask = int(GodotErrorMask::MASK_ALL_EXCEPT_WARNING);
	BitField<GodotErrorMask> error_logger_breadcrumb_mask = int(GodotErrorMask::MASK_ALL);
	Ref<SentryLoggerLimits> error_logger_limits;

	String configuration_script;
	Callable before_send;
	Callable on_crash;

	static void _define_project_settings(const Ref<SentryOptions> &p_options);
	static void _load_project_settings(const Ref<SentryOptions> &p_options);

	void _init_debug_option(DebugMode p_debug_mode);

	void _set_scene_tree_extra_properties(const PackedStringArray &p_scene_tree_extra_properties);
	PackedStringArray _get_scene_tree_extra_properties() const;

protected:
	static void _bind_methods();

public:
	static void create_singleton();
	static void destroy_singleton();
	_FORCE_INLINE_ static Ref<SentryOptions> get_singleton() { return singleton; }

	_FORCE_INLINE_ bool is_enabled() const { return enabled; }
	_FORCE_INLINE_ void set_enabled(bool p_enabled) { enabled = p_enabled; }

	_FORCE_INLINE_ bool is_disabled_in_editor() const { return disabled_in_editor; }
	_FORCE_INLINE_ void set_disabled_in_editor(bool p_disabled_in_editor) { disabled_in_editor = p_disabled_in_editor; }

	_FORCE_INLINE_ String get_dsn() const { return dsn; }
	_FORCE_INLINE_ void set_dsn(const String &p_dsn) { dsn = p_dsn; }

	_FORCE_INLINE_ String get_release() const { return release; }
	_FORCE_INLINE_ void set_release(const String &p_release) { release = p_release; }

	_FORCE_INLINE_ void set_dist(const String &p_dist) { dist = p_dist; }
	_FORCE_INLINE_ String get_dist() const { return dist; }

	_FORCE_INLINE_ String get_environment() const { return environment; }
	_FORCE_INLINE_ void set_environment(const String &p_environment) { environment = p_environment; }

	_FORCE_INLINE_ bool is_debug_enabled() const { return debug; }
	_FORCE_INLINE_ void set_debug_enabled(bool p_enabled) { debug = p_enabled; }

	_FORCE_INLINE_ void set_debug_verbosity(sentry::Level p_debug_verbosity) { debug_verbosity = p_debug_verbosity; }
	_FORCE_INLINE_ sentry::Level get_debug_verbosity() const { return debug_verbosity; }

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

	_FORCE_INLINE_ void set_attach_scene_tree_info(bool p_attach_scene_tree_info) { attach_scene_tree_info = p_attach_scene_tree_info; }
	_FORCE_INLINE_ bool is_attach_scene_tree_info_enabled() const { return attach_scene_tree_info; }

	_FORCE_INLINE_ Vector<StringName> get_scene_tree_extra_properties() const { return scene_tree_extra_properties; }

	_FORCE_INLINE_ bool is_error_logger_enabled() const { return error_logger_enabled; }
	_FORCE_INLINE_ void set_error_logger_enabled(bool p_enabled) { error_logger_enabled = p_enabled; }

	_FORCE_INLINE_ bool is_error_logger_include_source_enabled() const { return error_logger_include_source; }
	_FORCE_INLINE_ void set_error_logger_include_source(bool p_error_logger_include_source) { error_logger_include_source = p_error_logger_include_source; }

	_FORCE_INLINE_ BitField<GodotErrorMask> get_error_logger_event_mask() const { return error_logger_event_mask; }
	_FORCE_INLINE_ void set_error_logger_event_mask(BitField<GodotErrorMask> p_error_logger_event_mask) { error_logger_event_mask = p_error_logger_event_mask; }

	_FORCE_INLINE_ BitField<GodotErrorMask> get_error_logger_breadcrumb_mask() const { return error_logger_breadcrumb_mask; }
	_FORCE_INLINE_ void set_error_logger_breadcrumb_mask(BitField<GodotErrorMask> p_error_logger_breadcrumb_mask) { error_logger_breadcrumb_mask = p_error_logger_breadcrumb_mask; }

	_FORCE_INLINE_ bool is_error_logger_event_enabled(GodotErrorType p_error_type) { return error_logger_event_mask.has_flag(sentry::godot_error_type_as_mask(p_error_type)); }
	_FORCE_INLINE_ bool is_error_logger_breadcrumb_enabled(GodotErrorType p_error_type) { return error_logger_breadcrumb_mask.has_flag(sentry::godot_error_type_as_mask(p_error_type)); }

	_FORCE_INLINE_ Ref<SentryLoggerLimits> get_error_logger_limits() const { return error_logger_limits; }
	void set_error_logger_limits(const Ref<SentryLoggerLimits> &p_limits);

	_FORCE_INLINE_ String get_configuration_script() const { return configuration_script; }

	_FORCE_INLINE_ Callable get_before_send() const { return before_send; }
	_FORCE_INLINE_ void set_before_send(const Callable &p_before_send) { before_send = p_before_send; }

	_FORCE_INLINE_ Callable get_on_crash() const { return on_crash; }
	_FORCE_INLINE_ void set_on_crash(const Callable &p_on_crash) { on_crash = p_on_crash; }

	SentryOptions();
	~SentryOptions();
};

VARIANT_BITFIELD_CAST(SentryOptions::GodotErrorMask);

#endif // SENTRY_OPTIONS_H
