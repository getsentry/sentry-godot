#ifndef EXPERIMENTAL_LOGGER_H
#define EXPERIMENTAL_LOGGER_H

#include <fstream>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>

class ExperimentalLogger : public godot::Node {
	GDCLASS(ExperimentalLogger, godot::Node)
private:
	godot::Callable process_log;
	std::ifstream log_file;

	void _process_log_file();

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	void setup(const godot::String &p_log_path);
	ExperimentalLogger();
};

#endif // EXPERIMENTAL_LOGGER_H
