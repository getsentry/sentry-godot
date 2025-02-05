#include "runtime_config.h"

namespace {

inline String _ensure_string(const Variant &p_value, const String &p_fallback) {
	return p_value.get_type() == Variant::STRING ? (String)p_value : p_fallback;
}

} // unnamed namespace

void RuntimeConfig::set_installation_id(const String &p_id) {
	ERR_FAIL_COND(p_id.length() == 0);

	installation_id = p_id;
	conf->set_value("main", "installation_id", (String)installation_id);
	conf->save(conf_path);
}

void RuntimeConfig::load_file(const String &p_conf_path) {
	ERR_FAIL_COND(p_conf_path.is_empty());

	conf_path = p_conf_path;
	conf->load(conf_path);

	installation_id = _ensure_string(conf->get_value("main", "installation_id", ""), "");
}

RuntimeConfig::RuntimeConfig() {
	conf.instantiate();
}
