#include "sentry_user.h"

#include "sentry/uuid.h"

#include <godot_cpp/variant/packed_string_array.hpp>

namespace sentry {

bool SentryUser::is_empty() const {
	return id.is_empty() && username.is_empty() && email.is_empty() && ip_address.is_empty();
}

String SentryUser::_to_string() const {
	PackedStringArray parts;
	if (!id.is_empty()) {
		parts.append("id: " + id);
	}
	if (!username.is_empty()) {
		parts.append("name: " + username);
	}
	if (!email.is_empty()) {
		parts.append("email: " + email);
	}
	if (!ip_address.is_empty()) {
		parts.append("ip: " + ip_address);
	}
	return "SentryUser:{ " + String("; ").join(parts) + " }";
}

void SentryUser::generate_new_id() {
	id = sentry::uuid::make_uuid();
}

Ref<SentryUser> SentryUser::duplicate() {
	Ref<SentryUser> copy;
	copy.instantiate();
	copy->set_id(id);
	copy->set_username(username);
	copy->set_email(email);
	copy->set_ip_address(ip_address);
	return copy;
}

void SentryUser::_bind_methods() {
	// Setters / getters
	ClassDB::bind_method(D_METHOD("set_id", "id"), &SentryUser::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &SentryUser::get_id);
	ClassDB::bind_method(D_METHOD("set_username", "username"), &SentryUser::set_username);
	ClassDB::bind_method(D_METHOD("get_username"), &SentryUser::get_username);
	ClassDB::bind_method(D_METHOD("set_email", "email"), &SentryUser::set_email);
	ClassDB::bind_method(D_METHOD("get_email"), &SentryUser::get_email);
	ClassDB::bind_method(D_METHOD("set_ip_address", "ip_address"), &SentryUser::set_ip_address);
	ClassDB::bind_method(D_METHOD("get_ip_address"), &SentryUser::get_ip_address);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "id"), "set_id", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "username"), "set_username", "get_username");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "email"), "set_email", "get_email");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ip_address"), "set_ip_address", "get_ip_address");

	// Other methods
	ClassDB::bind_method(D_METHOD("infer_ip_address"), &SentryUser::infer_ip_address);
	ClassDB::bind_method(D_METHOD("is_empty"), &SentryUser::is_empty);
	ClassDB::bind_method(D_METHOD("generate_new_id"), &SentryUser::generate_new_id);
	ClassDB::bind_method(D_METHOD("duplicate"), &SentryUser::duplicate);
}

} // namespace sentry
