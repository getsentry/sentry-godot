#include "runtime_config.h"
#include "sentry_user.h"

#include <godot_cpp/core/memory.hpp>

namespace {

inline String _ensure_string(const Variant &p_value, const String &p_fallback) {
	return p_value.get_type() == Variant::STRING ? (String)p_value : p_fallback;
}

} // unnamed namespace

void RuntimeConfig::set_user(const Ref<SentryUser> &p_user) {
	user = p_user;

	conf->set_value("user", "id", p_user->get_user_id());
	conf->set_value("user", "email", p_user->get_email());
	conf->set_value("user", "username", p_user->get_username());
	conf->save(conf_path);
}

void RuntimeConfig::load_file(const String &p_conf_path) {
	conf_path = p_conf_path;
	conf->load(conf_path);

	user = Ref(memnew(SentryUser));
	user->set_user_id(_ensure_string(conf->get_value("user", "id"), ""));
	user->set_email(_ensure_string(conf->get_value("user", "email"), ""));
	user->set_username(_ensure_string(conf->get_value("user", "username"), ""));
}

RuntimeConfig::RuntimeConfig() {
	conf.instantiate();
}
