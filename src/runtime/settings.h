#ifndef SETTINGS_H
#define SETTINGS_H

#include <godot_cpp/variant/char_string.hpp>

class Settings {
private:
	static Settings *singleton;

	godot::CharString db_path;
	godot::CharString dsn;
	godot::CharString release;

	void _load_config();

public:
	static Settings *get_singleton() { return singleton; }

	godot::CharString get_release() const { return release; }
	godot::CharString get_dsn() const { return dsn; }
	godot::CharString get_db_path() const { return db_path; }

	Settings();
	~Settings();
};

#endif // SETTINGS_H
