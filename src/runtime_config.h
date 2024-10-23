#ifndef RUNTIME_CONFIG_H
#define RUNTIME_CONFIG_H

#include "sentry_user.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/variant/char_string.hpp>

using namespace godot;

class RuntimeConfig {
private:
	String conf_path;
	Ref<ConfigFile> conf;

	// Cached values.
	Ref<SentryUser> user;
	CharString device_id;

public:
	Ref<SentryUser> get_user() const { return user; }
	void set_user(const Ref<SentryUser> &p_user);

	CharString get_device_id() const { return device_id; }
	void set_device_id(const CharString &p_device_id);

	void load_file(const String &p_conf_path);

	RuntimeConfig();
};

#endif // RUNTIME_CONFIG_H