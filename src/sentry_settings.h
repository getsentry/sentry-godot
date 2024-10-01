#ifndef SENTRY_SETTINGS_H
#define SENTRY_SETTINGS_H

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

class SentrySettings {
private:
	static SentrySettings *singleton;

	bool sentry_enabled = true;
	godot::CharString dsn = "";
	godot::CharString release = "{app_name}@{app_version}";
	bool debug_printing = false;
	double sample_rate = 1.0;
	bool attach_log = true;
	int32_t config_value_order = 0;
	int max_breadcrumbs = 100;

	void _define_setting(const godot::String &p_setting, const godot::Variant &p_default, bool p_basic = true);
	void _define_setting(const godot::PropertyInfo &p_info, const godot::Variant &p_default, bool p_basic = true);
	void _define_project_settings();
	void _load_config();

public:
	static SentrySettings *get_singleton() { return singleton; }

	bool is_sentry_enabled() const { return sentry_enabled; }
	godot::CharString get_dsn() const { return dsn; }
	godot::CharString get_release() const { return release; }
	bool is_debug_printing_enabled() const { return debug_printing; }
	double get_sample_rate() const { return sample_rate; }
	bool is_attach_log_enabled() const { return attach_log; }
	int get_max_breadcrumbs() const { return max_breadcrumbs; }

	SentrySettings();
	~SentrySettings();
};

#endif // SENTRY_SETTINGS_H
