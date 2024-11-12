#ifndef SENTRY_OPTIONS_H
#define SENTRY_OPTIONS_H

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

class SentryOptions {
private:
	static SentryOptions *singleton;

	bool enabled = true;
	CharString dsn = "";
	CharString release = "{app_name}@{app_version}";
	bool debug = false;
	double sample_rate = 1.0;
	bool attach_log = true;
	int32_t config_value_order = 0;
	int max_breadcrumbs = 100;

	bool error_logger_enabled = true;
	bool error_logger_capture_as_breadcrumb = true;
	bool error_logger_capture_as_event = false;
	int error_logger_max_lines = 30;
	bool error_logger_log_warnings = true; // Note: Godot warnings are captured as breadcrumbs if this option is enabled.
	bool error_logger_include_source = true;

	void _define_setting(const String &p_setting, const Variant &p_default, bool p_basic = true);
	void _define_setting(const PropertyInfo &p_info, const Variant &p_default, bool p_basic = true);
	void _define_project_settings();
	void _load_project_settings();

public:
	_FORCE_INLINE_ static SentryOptions *get_singleton() { return singleton; }

	_FORCE_INLINE_ bool is_enabled() const { return enabled; }
	CharString get_dsn() const { return dsn; }
	CharString get_release() const { return release; }
	_FORCE_INLINE_ bool is_debug_enabled() const { return debug; }
	double get_sample_rate() const { return sample_rate; }
	bool is_attach_log_enabled() const { return attach_log; }
	int get_max_breadcrumbs() const { return max_breadcrumbs; }

	bool is_error_logger_enabled() const { return error_logger_enabled; }
	bool is_error_logger_capture_as_breadcrumb_enabled() const { return error_logger_capture_as_breadcrumb; }
	bool is_error_logger_capture_as_event_enabled() const { return error_logger_capture_as_event; }
	int get_error_logger_max_lines() const { return error_logger_max_lines; }
	bool is_error_logger_log_warnings_enabled() const { return error_logger_log_warnings; }
	bool is_error_logger_include_source_enabled() const { return error_logger_include_source; }

	SentryOptions();
	~SentryOptions();
};

#endif // SENTRY_OPTIONS_H
