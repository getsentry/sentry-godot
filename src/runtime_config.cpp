#include "runtime_config.h"
#include "sentry_user.h"

#include <godot_cpp/core/memory.hpp>

namespace {

inline String _ensure_string(const Variant &p_value, const String &p_fallback) {
	return p_value.get_type() == Variant::STRING ? (String)p_value : p_fallback;
}

inline CharString _ensure_cstring(const Variant &p_value, const CharString &p_fallback) {
	return p_value.get_type() == Variant::STRING ? ((String)p_value).utf8() : p_fallback;
}

} // unnamed namespace

void RuntimeConfig::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_COND(p_user.is_null());
	user = p_user;

	conf->set_value("user", "id", p_user->get_id());
	conf->set_value("user", "email", p_user->get_email());
	conf->set_value("user", "username", p_user->get_username());
	conf->save(conf_path);
}

void RuntimeConfig::set_device_id(const String &p_device_id) {
	ERR_FAIL_COND(p_device_id.length() == 0);
	device_id = p_device_id;
	conf->set_value("device", "id", (String)device_id);
	conf->save(conf_path);
}

void RuntimeConfig::load_file(const String &p_conf_path) {
	ERR_FAIL_COND(p_conf_path.is_empty());

	conf_path = p_conf_path;
	conf->load(conf_path);

	user = Ref(memnew(SentryUser));
	user->set_id(_ensure_string(conf->get_value("user", "id", ""), ""));
	user->set_email(_ensure_string(conf->get_value("user", "email", ""), ""));
	user->set_username(_ensure_string(conf->get_value("user", "username", ""), ""));

	device_id = _ensure_cstring(conf->get_value("device", "id", ""), "");
}

RuntimeConfig::RuntimeConfig() {
	conf.instantiate();
}
