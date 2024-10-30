#ifndef SENTRY_OPTIONS_H
#define SENTRY_OPTIONS_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

class SentryOptions {
public:
	enum CaptureType {
		CAPTURE_AS_BREADCRUMB,
		CAPTURE_AS_EVENT,
	};

private:
	static SentryOptions *singleton;

	bool enabled = true;
	godot::CharString dsn = "";
	godot::CharString release = "{app_name}@{app_version}";
	bool debug = false;
	double sample_rate = 1.0;
	bool attach_log = true;
	int32_t config_value_order = 0;
	int max_breadcrumbs = 100;

	bool error_logger_enabled = true;
	int error_logger_max_lines = 30;
	bool error_logger_log_warnings = true; // Note: Godot warnings are captured as breadcrumbs if this option is enabled.
	CaptureType error_logger_capture_type = CaptureType::CAPTURE_AS_BREADCRUMB;
	bool error_logger_include_source = true;

	void _define_setting(const godot::String &p_setting, const godot::Variant &p_default, bool p_basic = true);
	void _define_setting(const godot::PropertyInfo &p_info, const godot::Variant &p_default, bool p_basic = true);
	void _define_project_settings();
	void _load_project_settings();

public:
	_FORCE_INLINE_ static SentryOptions *get_singleton() { return singleton; }

	_FORCE_INLINE_ bool is_enabled() const { return enabled; }
	godot::CharString get_dsn() const { return dsn; }
	godot::CharString get_release() const { return release; }
	_FORCE_INLINE_ bool is_debug_enabled() const { return debug; }
	double get_sample_rate() const { return sample_rate; }
	bool is_attach_log_enabled() const { return attach_log; }
	int get_max_breadcrumbs() const { return max_breadcrumbs; }
	bool is_error_logger_enabled() const { return error_logger_enabled; }
	int get_error_logger_max_lines() const { return error_logger_max_lines; }
	bool is_error_logger_log_warnings_enabled() const { return error_logger_log_warnings; }
	CaptureType get_error_logger_capture_type() const { return error_logger_capture_type; }
	bool is_error_logger_include_source_enabled() const { return error_logger_include_source; }

	SentryOptions();
	~SentryOptions();
};

#endif // SENTRY_OPTIONS_H
