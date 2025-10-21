#pragma once

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

class SentryFeedback : public RefCounted {
	GDCLASS(SentryFeedback, RefCounted);

private:
	String name;
	String contact_email;
	String message;
	String associated_event_id;

protected:
	static void _bind_methods();

public:
	String get_name() const { return name; }
	void set_name(const String &p_name) { name = p_name; }

	String get_contact_email() const { return contact_email; }
	void set_contact_email(const String &p_contact_email) { contact_email = p_contact_email; }

	String get_message() const { return message; }
	void set_message(const String &p_message) { message = p_message; }

	String get_associated_event_id() const { return associated_event_id; }
	void set_associated_event_id(const String &p_associated_event_id) { associated_event_id = p_associated_event_id; }
};

} //namespace sentry
