#include "sentry_user.h"

using namespace godot;

bool SentryUser::is_user_valid() const {
	return !user_id.is_empty() || !username.is_empty() || !email.is_empty() || !ip_address.is_empty();
}

void SentryUser::_bind_methods() {
	// Setters / getters
	ClassDB::bind_method(D_METHOD("set_user_id", "id"), &SentryUser::set_user_id);
	ClassDB::bind_method(D_METHOD("get_user_id"), &SentryUser::get_user_id);
	ClassDB::bind_method(D_METHOD("set_username", "username"), &SentryUser::set_username);
	ClassDB::bind_method(D_METHOD("get_username"), &SentryUser::get_username);
	ClassDB::bind_method(D_METHOD("set_email", "email"), &SentryUser::set_email);
	ClassDB::bind_method(D_METHOD("get_email"), &SentryUser::get_email);
	ClassDB::bind_method(D_METHOD("set_ip_address", "ip_address"), &SentryUser::set_ip_address);
	ClassDB::bind_method(D_METHOD("get_ip_address"), &SentryUser::get_ip_address);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "user_id"), "set_user_id", "get_user_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "username"), "set_username", "get_username");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "email"), "set_email", "get_email");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ip_address"), "set_ip_address", "get_ip_address");

	// Other methods
	ClassDB::bind_method(D_METHOD("infer_ip_address"), &SentryUser::infer_ip_address);
	ClassDB::bind_method(D_METHOD("is_user_valid"), &SentryUser::is_user_valid);
}
