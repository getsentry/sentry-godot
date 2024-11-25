#ifndef SENTRY_OPTIONS_H
#define SENTRY_OPTIONS_H

#include "sentry/godot_error_types.h"

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

class SentryOptions {
	using GodotErrorType = sentry::GodotErrorType;
	using GodotErrorMask = sentry::GodotErrorMask;

public:
	struct LoggerLimits {
		int max_lines_parsed = 30;
		int breadcrumbs_per_frame = 10;
		int events_per_frame = 2;
		int repeated_error_throttling_ms = 1000;
	};

private:
	static SentryOptions *singleton;

	bool enabled = true;
	bool disabled_in_editor = true;
	CharString dsn = "";
	CharString release = "{app_name}@{app_version}";
	bool debug = false;
	double sample_rate = 1.0;
	bool attach_log = true;
	int32_t config_value_order = 0;
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

public:
	_FORCE_INLINE_ static SentryOptions *get_singleton() { return singleton; }

	_FORCE_INLINE_ bool is_enabled() const { return enabled; }
	_FORCE_INLINE_ bool is_disabled_in_editor() const { return disabled_in_editor; }
	CharString get_dsn() const { return dsn; }
	CharString get_release() const { return release; }
	_FORCE_INLINE_ bool is_debug_enabled() const { return debug; }
	double get_sample_rate() const { return sample_rate; }
	bool is_attach_log_enabled() const { return attach_log; }
	int get_max_breadcrumbs() const { return max_breadcrumbs; }
	bool is_send_default_pii_enabled() const { return send_default_pii; }

	bool is_error_logger_enabled() const { return error_logger_enabled; }
	bool is_error_logger_include_source_enabled() const { return error_logger_include_source; }
	bool is_error_logger_event_enabled(GodotErrorType p_error_type) { return error_logger_event_mask & sentry::godot_error_type_as_mask(p_error_type); }
	bool is_error_logger_breadcrumb_enabled(GodotErrorType p_error_type) { return error_logger_breadcrumb_mask & sentry::godot_error_type_as_mask(p_error_type); }
	LoggerLimits get_error_logger_limits() const { return error_logger_limits; }

	SentryOptions();
	~SentryOptions();
};

#endif // SENTRY_OPTIONS_H
