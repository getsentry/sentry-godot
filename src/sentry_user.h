#ifndef SENTRY_USER_H
#define SENTRY_USER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class SentryUser : public RefCounted {
	GDCLASS(SentryUser, RefCounted);

private:
	String user_id;
	String username;
	String email;
	String ip_address;

protected:
	static void _bind_methods();

	String _to_string() const;

public:
	void set_user_id(String p_user_id) { user_id = p_user_id; }
	String get_user_id() const { return user_id; }

	void set_username(String p_username) { username = p_username; }
	String get_username() const { return username; }

	void set_email(String p_email) { email = p_email; }
	String get_email() const { return email; }

	void set_ip_address(String p_ip_address) { ip_address = p_ip_address; }
	String get_ip_address() const { return ip_address; }

	void infer_ip_address() { ip_address = "{{auto}}"; }

	void assign_unique_id();

	bool is_user_valid() const;
};

#endif // SENTRY_USER_H
