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

const int num_error_types = sizeof(error_types) / sizeof(error_types[0]);

} //namespace

void ExperimentalLogger::_process_log_file() {
	auto start = std::chrono::high_resolution_clock::now();

	if (!log_file.is_open()) {
		set_process(false);
		ERR_FAIL_MSG("Sentry: Internal error: Log file not open.");
		return;
	}

	log_file.clear();

	int num_lines_read = 0;
	char line[MAX_LINE_LENGTH];
	char message[MAX_LINE_LENGTH * 2 + 2];

	while (num_lines_read < 20 && log_file.getline(line, MAX_LINE_LENGTH)) {
		num_lines_read++;

		for (int i = 0; i < num_error_types; i++) {
			if (strncmp(line, error_types[i], strlen(error_types[i])) == 0) {
				// std::cout << DEBUG_PREFIX << "CAUGHT ERROR" << std::endl;

				strcpy(message, line);
				strcat(message, "\n");

				if (log_file.getline(line, MAX_LINE_LENGTH)) {
					num_lines_read++;
					strcat(message, line);
					strcat(message, "\n");
				}

				sentry_value_t crumb = sentry_value_new_breadcrumb("default", message);
				sentry_value_set_by_key(crumb, "category", sentry_value_new_string("godot.logger"));
				sentry_value_set_by_key(crumb, "level", sentry_value_new_string("error"));
				sentry_add_breadcrumb(crumb);

				break;
			}
		}
	}

	// Seek to the end of file - don't process the rest of the lines.
	log_file.seekg(0, std::ios::end);

	// std::cout << DEBUG_PREFIX << "Lines read: " << num_lines_read << std::endl;

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << DEBUG_PREFIX << "Godot log processing took " << duration << " usec" << std::endl;
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
