#include "experimental_logger.h"

#include "sentry.h"
#include "sentry_options.h"
#include "sentry_singleton.h"

#include <cstring>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

namespace {

#define DEBUG_PREFIX "[sentry] DEBUG "
#define MAX_LINE_LENGTH 1024

const char *error_types[] = {
	"ERROR",
	"WARNING",
	"SCRIPT ERROR",
	"SHADER ERROR",
	"USER ERROR",
	"USER WARNING",
	"USER SCRIPT ERROR",
	"USER SHADER ERROR"
};

ExperimentalLogger::ErrorType error_type_as_enum[] = {
	ExperimentalLogger::ERROR_TYPE_ERROR,
	ExperimentalLogger::ERROR_TYPE_WARNING,
	ExperimentalLogger::ERROR_TYPE_SCRIPT,
	ExperimentalLogger::ERROR_TYPE_SHADER,
	ExperimentalLogger::ERROR_TYPE_ERROR,
	ExperimentalLogger::ERROR_TYPE_WARNING,
	ExperimentalLogger::ERROR_TYPE_SCRIPT,
	ExperimentalLogger::ERROR_TYPE_SHADER,
};

const int num_error_types = sizeof(error_types) / sizeof(error_types[0]);

} //namespace

void ExperimentalLogger::_process_log_file() {
	// TODO: Remove the following block before merge.
	auto start = std::chrono::high_resolution_clock::now();

	if (!log_file.is_open()) {
		set_process(false);
		ERR_FAIL_MSG("Sentry: Internal error: Log file not open.");
		return;
	}

	log_file.clear();

	int num_lines_read = 0;
	char first_line[MAX_LINE_LENGTH];
	char second_line[MAX_LINE_LENGTH];
	int max_lines = SentryOptions::get_singleton()->get_error_logger_max_lines();

	while (num_lines_read < max_lines && log_file.getline(first_line, MAX_LINE_LENGTH)) {
		num_lines_read++;

		for (int i = 0; i < num_error_types; i++) {
			if (strncmp(first_line, error_types[i], strlen(error_types[i])) == 0) {
				if (!log_file.getline(second_line, MAX_LINE_LENGTH)) {
					continue;
				}
				num_lines_read++;

				// Parse error string.
				// See: https://github.com/godotengine/godot/blob/04692d83cb8f61002f18ea1d954df8c558ee84f7/core/io/logger.cpp#L88
				ErrorType err_type = error_type_as_enum[i];
				char *rationale = first_line + strlen(error_types[i]) + 2; // +2 to skip ": "
				char func[100];
				char file_part[200];
				int parsed = sscanf(second_line, "   at: %99s (%199[^)])\n", func, file_part);
				if (parsed == 2) {
					// Split file name and line number.
					char *last_colon = strrchr(file_part, ':');
					if (last_colon != NULL) {
						*last_colon = '\0';
						int line = atoi(last_colon + 1);
						_log_error(func, file_part, line, rationale, err_type);
					}
				}

				break;
			}
		}
	}

	// Seek to the end of file - don't process the rest of the lines.
	log_file.seekg(0, std::ios::end);

	// TODO: Remove the following block before merge.
	// std::cout << DEBUG_PREFIX << "Lines read: " << num_lines_read << std::endl;
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	// std::cout << DEBUG_PREFIX << "Godot log processing took " << duration << " usec" << std::endl;
}

void ExperimentalLogger::_log_error(const char *p_func, const char *p_file, int p_line, const char *p_rationale, ErrorType p_error_type) {
	// TODO: Remove the following block before merge.
	printf("[sentry] DEBUG Godot error caught:\n");
	printf("   Function: \"%s\"\n", p_func);
	printf("   File: \"%s\"\n", p_file);
	printf("   Line: %d\n", p_line);
	printf("   Rationale: \"%s\"\n", p_rationale);
	printf("   Error Type: %d\n", p_error_type);

	// * Note: Currently, added as a breadcrumb until a proper mechanism is established.
	sentry_value_t crumb = sentry_value_new_breadcrumb("error", p_rationale);
	sentry_value_set_by_key(crumb, "category", sentry_value_new_string("error"));
	sentry_value_set_by_key(crumb, "level",
			sentry_value_new_string(
					p_error_type == ERROR_TYPE_WARNING ? "warning" : "error"));

	sentry_value_t data = sentry_value_new_object();
	sentry_value_set_by_key(data, "function", sentry_value_new_string(p_func));
	sentry_value_set_by_key(data, "file", sentry_value_new_string(p_file));
	sentry_value_set_by_key(data, "line", sentry_value_new_int32(p_line));

	const char *error_string;
	switch (p_error_type) {
		case ERROR_TYPE_WARNING: {
			error_string = "WARNING";
		} break;
		case ERROR_TYPE_SCRIPT: {
			error_string = "SCRIPT ERROR";
		} break;
		case ERROR_TYPE_SHADER: {
			error_string = "SHADER ERROR";
		} break;
		case ERROR_TYPE_ERROR:
		default: {
			error_string = "ERROR";
		} break;
	}
	sentry_value_set_by_key(data, "godot_error_type", sentry_value_new_string(error_string));

	sentry_value_set_by_key(crumb, "data", data);
	sentry_add_breadcrumb(crumb);
}

void ExperimentalLogger::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			// Process log file at the end of the current frame.
			process_log.call_deferred();
		} break;
	}
}

void ExperimentalLogger::setup(const String &p_log_path) {
	ERR_FAIL_COND(p_log_path.is_empty());
	process_log = callable_mp(this, &ExperimentalLogger::_process_log_file);
	String log_path = p_log_path.replace("user://", OS::get_singleton()->get_user_data_dir() + "/");
	log_file.open(log_path.utf8(), std::ios::in);
	set_process(log_file.is_open());
	ERR_FAIL_COND_MSG(!log_file.is_open(), "Sentry: Failed to open log file: " + p_log_path);
}

ExperimentalLogger::ExperimentalLogger() {
	set_process(false);
}
