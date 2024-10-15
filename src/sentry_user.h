#ifndef SENTRY_USER_H
#define SENTRY_USER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

class SentryUser : public godot::RefCounted {
	GDCLASS(SentryUser, godot::RefCounted);

private:
	godot::String user_id;
	godot::String username;
	godot::String email;
	godot::String ip_address;

protected:
	static void _bind_methods();

public:
	void set_user_id(godot::String p_user_id) { user_id = p_user_id; }
	godot::String get_user_id() const { return user_id; }

	void set_username(godot::String p_username) { username = p_username; }
	godot::String get_username() const { return username; }

	void set_email(godot::String p_email) { email = p_email; }
	godot::String get_email() const { return email; }

	void set_ip_address(godot::String p_ip_address) { ip_address = p_ip_address; }
	godot::String get_ip_address() const { return ip_address; }

	void infer_ip_address() { ip_address = "{{auto}}"; }

	bool is_user_valid() const;
};

#endif // SENTRY_USER_H
