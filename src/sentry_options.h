#ifndef SENTRY_OPTIONS_H
#define SENTRY_OPTIONS_H

#include "sentry/godot_error_types.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

class SentryOptions : public RefCounted {
	GDCLASS(SentryOptions, RefCounted);

	using GodotErrorType = sentry::GodotErrorType;
	using GodotErrorMask = sentry::GodotErrorMask;

public:
	struct LoggerLimits {
		// Limit the number of lines that can be parsed per frame.
		int parse_lines = 100;

		// Protect frametime budget.
		int events_per_frame = 5;

		// Limit to 1 error captured per source line within T milliseconds window.
		int repeated_error_window_ms = 1000;

		// Limit to N events within T milliseconds window.
		int throttle_events = 20;
		int throttle_window_ms = 10000;
	};

private:
	static SentryOptions *singleton;

	int32_t config_value_order = 0;

	bool enabled = true;
	bool disabled_in_editor = true;
	String dsn = "";
	String release = "{app_name}@{app_version}";
	bool debug = false;
	double sample_rate = 1.0;
	bool attach_log = true;
	int max_breadcrumbs = 100;
	bool send_default_pii = false;

	bool error_logger_enabled = true;
	bool error_logger_include_source = true;
	int error_logger_event_mask = int(GodotErrorMask::MASK_ALL_EXCEPT_WARNING);
	int error_logger_breadcrumb_mask = int(GodotErrorMask::MASK_ALL);
	LoggerLimits error_logger_limits;

	void _define_setting(const String &p_setting, const Variant &p_default, bool p_basic = true);
	void _define_setting(const PropertyInfo &p_info, const Variant &p_default, bool p_basic = true);
	void _define_project_settings();
	void _load_project_settings();

protected:
	static void _bind_methods();

public:
	_FORCE_INLINE_ static SentryOptions *get_singleton() { return singleton; }

	_FORCE_INLINE_ bool is_enabled() const { return enabled; }
	_FORCE_INLINE_ void set_enabled(bool p_enabled) { enabled = p_enabled; }

	_FORCE_INLINE_ bool is_disabled_in_editor() const { return disabled_in_editor; }
	_FORCE_INLINE_ void set_disabled_in_editor(bool p_disabled_in_editor) { disabled_in_editor = p_disabled_in_editor; }

	_FORCE_INLINE_ String get_dsn() const { return dsn; }
	_FORCE_INLINE_ void set_dsn(const String &p_dsn) { dsn = p_dsn; }

	_FORCE_INLINE_ String get_release() const { return release; }
	_FORCE_INLINE_ void set_release(const String &p_release) { release = p_release; }

	_FORCE_INLINE_ bool is_debug_enabled() const { return debug; }
	_FORCE_INLINE_ void set_debug_enabled(bool p_debug) { debug = p_debug; }

	_FORCE_INLINE_ double get_sample_rate() const { return sample_rate; }
	_FORCE_INLINE_ void set_sample_rate(double p_sample_rate) { sample_rate = p_sample_rate; }

	_FORCE_INLINE_ bool is_attach_log_enabled() const { return attach_log; }
	_FORCE_INLINE_ void set_attach_log(bool p_enabled) { attach_log = p_enabled; }

	_FORCE_INLINE_ int get_max_breadcrumbs() const { return max_breadcrumbs; }
	_FORCE_INLINE_ void set_max_breadcrumbs(int p_max_breadcrumbs) { max_breadcrumbs = p_max_breadcrumbs; }

	_FORCE_INLINE_ bool is_send_default_pii_enabled() const { return send_default_pii; }
	_FORCE_INLINE_ void set_send_default_pii(bool p_enabled) { send_default_pii = p_enabled; }

	_FORCE_INLINE_ bool is_error_logger_enabled() const { return error_logger_enabled; }
	_FORCE_INLINE_ void set_error_logger_enabled(bool p_enabled) { error_logger_enabled = p_enabled; }

	_FORCE_INLINE_ bool is_error_logger_include_source_enabled() const { return error_logger_include_source; }
	_FORCE_INLINE_ void set_error_logger_include_source(bool p_error_logger_include_source) { error_logger_include_source = p_error_logger_include_source; }

	_FORCE_INLINE_ bool get_error_logger_event_mask() const { return error_logger_event_mask; }
	_FORCE_INLINE_ void set_error_logger_event_mask(int p_error_logger_event_mask) { error_logger_event_mask = p_error_logger_event_mask; }

	_FORCE_INLINE_ int get_error_logger_breadcrumb_mask() const { return error_logger_breadcrumb_mask; }
	_FORCE_INLINE_ void set_error_logger_breadcrumb_mask(int p_error_logger_breadcrumb_mask) { error_logger_breadcrumb_mask = p_error_logger_breadcrumb_mask; }

	_FORCE_INLINE_ bool is_error_logger_event_enabled(GodotErrorType p_error_type) { return error_logger_event_mask & sentry::godot_error_type_as_mask(p_error_type); }
	_FORCE_INLINE_ bool is_error_logger_breadcrumb_enabled(GodotErrorType p_error_type) { return error_logger_breadcrumb_mask & sentry::godot_error_type_as_mask(p_error_type); }

	LoggerLimits get_error_logger_limits() const { return error_logger_limits; }

	SentryOptions();
	~SentryOptions();
};

#endif // SENTRY_OPTIONS_H
