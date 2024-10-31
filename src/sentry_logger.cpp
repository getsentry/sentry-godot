#include "sentry_logger.h"

#include "sentry/native/native_util.h"
#include "sentry_options.h"
#include "sentry_sdk.h"
#include "sentry_util.h"

#include <sentry.h>
#include <cstring>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

namespace {

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

SentryLogger::ErrorType error_type_as_enum[] = {
	SentryLogger::ERROR_TYPE_ERROR,
	SentryLogger::ERROR_TYPE_WARNING,
	SentryLogger::ERROR_TYPE_SCRIPT,
	SentryLogger::ERROR_TYPE_SHADER,
	SentryLogger::ERROR_TYPE_ERROR,
	SentryLogger::ERROR_TYPE_WARNING,
	SentryLogger::ERROR_TYPE_SCRIPT,
	SentryLogger::ERROR_TYPE_SHADER,
};

const int num_error_types = sizeof(error_types) / sizeof(error_types[0]);

} //namespace

void SentryLogger::_process_log_file() {
	if (!log_file.is_open()) {
		set_process(false);
		ERR_FAIL_MSG("Sentry: Internal error: Log file not open. Error logging stopped.");
		return;
	}

	log_file.clear(); // Remove eof flag, so that we can read the next line.

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
}

void SentryLogger::_log_error(const char *p_func, const char *p_file, int p_line, const char *p_rationale, ErrorType p_error_type) {
	bool as_breadcrumb = false;
	if (p_error_type == ERROR_TYPE_WARNING) {
		if (SentryOptions::get_singleton()->is_error_logger_log_warnings_enabled()) {
			as_breadcrumb = true;
		} else {
			// Don't log if warnings are disabled.
			return;
		}
	} else {
		if (SentryOptions::get_singleton()->get_error_logger_capture_type() == SentryOptions::CAPTURE_AS_BREADCRUMB) {
			as_breadcrumb = true;
		}
	}

	// Debug output.
	if (SentryOptions::get_singleton()->is_debug_enabled()) {
		printf("[sentry] DEBUG GODOTSDK error logged:\n");
		printf("   Function: \"%s\"\n", p_func);
		printf("   File: \"%s\"\n", p_file);
		printf("   Line: %d\n", p_line);
		printf("   Rationale: \"%s\"\n", p_rationale);
		printf("   Error Type: %s\n", error_types[p_error_type]);
	}

	if (as_breadcrumb) {
		// Log error as breadcrumb.
		// TODO: Move this code to abstraction layer later when its available.
		sentry_value_t crumb = sentry_value_new_breadcrumb("error", p_rationale);
		sentry_value_set_by_key(crumb, "category", sentry_value_new_string("error"));
		sentry_value_set_by_key(crumb, "level",
				sentry_value_new_string(
						p_error_type == ERROR_TYPE_WARNING ? "warning" : "error"));

		sentry_value_t data = sentry_value_new_object();
		sentry_value_set_by_key(data, "function", sentry_value_new_string(p_func));
		sentry_value_set_by_key(data, "file", sentry_value_new_string(p_file));
		sentry_value_set_by_key(data, "line", sentry_value_new_int32(p_line));

		const char *error_string = error_types[p_error_type];
		sentry_value_set_by_key(data, "godot_error_type", sentry_value_new_string(error_string));

		sentry_value_set_by_key(crumb, "data", data);
		sentry_add_breadcrumb(crumb);
	} else {
		// Log error as event.
		// TODO: Move this code to abstraction layer later when its available.
		sentry_value_t event = sentry_value_new_event();
		sentry::Level sentry_level = p_error_type == ERROR_TYPE_WARNING ? sentry::LEVEL_WARNING : sentry::LEVEL_ERROR;
		sentry_value_set_by_key(event, "level",
				sentry_value_new_string(sentry::level_as_cstring(sentry_level)));

		sentry_value_t exception = sentry_value_new_exception(error_types[p_error_type], p_rationale);
		sentry_value_t stack_trace = sentry_value_new_object();

		sentry_value_t frames = sentry_value_new_list();
		sentry_value_t top_frame = sentry_value_new_object();
		sentry_value_set_by_key(top_frame, "filename", sentry_value_new_string(p_file));
		sentry_value_set_by_key(top_frame, "function", sentry_value_new_string(p_func));
		sentry_value_set_by_key(top_frame, "lineno", sentry_value_new_int32(p_line));
		sentry_value_append(frames, top_frame);

		if (p_error_type == ERROR_TYPE_SCRIPT && SentryOptions::get_singleton()->is_error_logger_include_source_enabled()) {
			// Provide script source code context for script errors if available.
			// TODO: Should it be optional?
			String context_line;
			PackedStringArray pre_context;
			PackedStringArray post_context;
			bool err = _get_script_context(p_file, p_line, context_line, pre_context, post_context);
			if (!err) {
				sentry_value_set_by_key(top_frame, "context_line", sentry_value_new_string(context_line.utf8()));
				sentry_value_set_by_key(top_frame, "pre_context", sentry::native::strings_to_sentry_list(pre_context));
				sentry_value_set_by_key(top_frame, "post_context", sentry::native::strings_to_sentry_list(post_context));
			}
		}

		sentry_value_set_by_key(stack_trace, "frames", frames);
		sentry_value_set_by_key(exception, "stacktrace", stack_trace);
		sentry_event_add_exception(event, exception);
		sentry_capture_event(event);
	}
}

bool SentryLogger::_get_script_context(const String &p_file, int p_line, String &r_context_line, PackedStringArray &r_pre_context, PackedStringArray &r_post_context) const {
	if (p_file.is_empty()) {
		return true;
	}

	Ref<Script> script = ResourceLoader::get_singleton()->load(p_file);

	// ! Note: Script source code context is only automatically provided if GDScript is exported as text (not binary tokens).

	if (script.is_null()) {
		SentryUtil::print_error("Failed to load script ", p_file);
		return true;
	}

	String source_code = script->get_source_code();
	if (source_code.is_empty()) {
		SentryUtil::print_debug("Script source not available ", p_file.utf8().ptr());
		return true;
	}

	PackedStringArray lines = script->get_source_code().split("\n");
	if (lines.size() < p_line) {
		SentryUtil::print_error("Script source is smaller than the referenced line, lineno: ", p_line);
		return true;
	}

	r_context_line = lines[p_line - 1];
	r_pre_context = lines.slice(MAX(p_line - 6, 0), p_line - 1);
	r_post_context = lines.slice(p_line, MIN(p_line + 5, lines.size()));
	return false;
}

void SentryLogger::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			SentryUtil::print_debug("starting logger");
			_setup();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			SentryUtil::print_debug("finishing logger");
			log_file.close();
		} break;
		case NOTIFICATION_PROCESS: {
			// Process log file at the end of the current frame.
			process_log.call_deferred();
		} break;
	}
}

void SentryLogger::_setup() {
	ERR_FAIL_NULL(ProjectSettings::get_singleton());
	ERR_FAIL_NULL(OS::get_singleton());

	bool logging_setting = ProjectSettings::get_singleton()->get_setting("debug/file_logging/enable_file_logging");
	bool logging_setting_pc = ProjectSettings::get_singleton()->get_setting("debug/file_logging/enable_file_logging.pc");
	bool logging_enabled;
#if defined(WINDOWS_ENABLED) || defined(LINUX_ENABLED) || defined(MACOS_ENABLED)
	logging_enabled = logging_setting_pc || logging_setting;
#else
	logging_enabled = logging_setting;
#endif
	ERR_FAIL_COND_MSG(!logging_enabled, "Sentry: Error logger failure - file logging disabled in project settings. Tip: Enable \"debug/file_logging/enable_file_logging\" in the project settings.");

	String log_path = ProjectSettings::get_singleton()->get_setting("debug/file_logging/log_path");
	ERR_FAIL_COND_MSG(log_path.is_empty(), "Sentry: Error logger failure - project settings \"debug/file_logging/log_path\" is not set. Please, assign a valid file path in the project settings.");
	log_path = log_path.replace("user://", OS::get_singleton()->get_user_data_dir() + "/");
	log_file.open(log_path.utf8(), std::ios::in);
	set_process(log_file.is_open());
	ERR_FAIL_COND_MSG(!log_file.is_open(), "Sentry: Error logger failure - couldn't open the log file: " + log_path);
}

SentryLogger::SentryLogger() {
	set_process(false);
	process_log = callable_mp(this, &SentryLogger::_process_log_file);
}
