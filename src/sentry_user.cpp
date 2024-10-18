#include "sentry_user.h"

#include <uuid.h>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

void SentryUser::assign_unique_id() {
	ERR_FAIL_COND_MSG(!user_id.is_empty(), "Sentry: User ID already assigned!");

	std::random_device rd;
	auto seed_data = std::array<int, std::mt19937::state_size>{};
	std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
	std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
	std::mt19937 generator(seq);
	uuids::uuid_random_generator gen{ generator };
	uuids::uuid const id = gen();

	user_id = uuids::to_string(id).c_str();
}

bool SentryUser::is_user_valid() const {
	return !user_id.is_empty() || !username.is_empty() || !email.is_empty() || !ip_address.is_empty();
}

godot::String SentryUser::_to_string() const {
	PackedStringArray parts;
	if (!user_id.is_empty()) {
		parts.append("id: " + user_id);
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
	return "SentryUser:{" + String("; ").join(parts) + "}";
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
	ClassDB::bind_method(D_METHOD("assign_unique_id"), &SentryUser::assign_unique_id);
}
