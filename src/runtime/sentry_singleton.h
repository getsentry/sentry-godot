#ifndef SENTRY_SINGLETON_H
#define SENTRY_SINGLETON_H

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

class Sentry : public godot::Object {
	GDCLASS(Sentry, godot::Object);

private:
	static Sentry *singleton;

protected:
	static void _bind_methods();

public:
	enum Level {
		LEVEL_DEBUG = -1,
		LEVEL_INFO = 0,
		LEVEL_WARNING = 1,
		LEVEL_ERROR = 2,
		LEVEL_FATAL = 3,
	};

	static Sentry *get_singleton() { return singleton; }

	godot::CharString get_environment() { return "production"; }
	godot::CharString get_level_cstring(Level p_level);

	void capture_message_event(const godot::String &p_message, Level p_level, const godot::String &p_logger = "");
	void add_breadcrumb(const godot::String &p_message, const godot::String &p_category, Level p_level,
			const godot::String &p_type, const godot::Dictionary &p_data);

	Sentry();
	~Sentry();
};

VARIANT_ENUM_CAST(Sentry::Level);

#endif // SENTRY_SINGLETON_H
