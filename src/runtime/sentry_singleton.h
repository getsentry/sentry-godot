#ifndef SENTRY_SINGLETON_H
#define SENTRY_SINGLETON_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/string.hpp>

#include <sentry.h>

class Sentry : public godot::Object {
	GDCLASS(Sentry, godot::Object);

private:
	static Sentry *singleton;

	sentry_options_t *options = nullptr;

protected:
	static void _bind_methods();

public:
	static Sentry *get_singleton() { return singleton; }

	godot::CharString get_environment() { return "production"; }

	void capture_message_event(const godot::String &p_text, const godot::String &p_logger = "");

	Sentry();
	~Sentry();
};

#endif // SENTRY_SINGLETON_H
