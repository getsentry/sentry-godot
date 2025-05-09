#include "sentry_logger.h"

#include "sentry/internal_sdk.h"
#include "sentry/util/print.h"
#include "sentry_options.h"
#include "sentry_sdk.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>

namespace {

// Error enum values as strings
const char *error_types[] = {
	"ERROR",
	"WARNING",
	"SCRIPT ERROR",
	"SHADER ERROR",
};

bool _get_script_context(const String &p_file, int p_line, String &r_context_line, PackedStringArray &r_pre_context, PackedStringArray &r_post_context) {
	if (p_file.is_empty()) {
		return true;
	}

	Ref<Script> script = ResourceLoader::get_singleton()->load(p_file);

	// ! Note: Script source code context is only automatically provided if GDScript is exported as text (not binary tokens).

	if (script.is_null()) {
		sentry::util::print_error("Failed to load script ", p_file);
		return true;
	}

	String source_code = script->get_source_code();
	if (source_code.is_empty()) {
		sentry::util::print_debug("Script source not available ", p_file.utf8().ptr());
		return true;
	}

	PackedStringArray lines = script->get_source_code().split("\n");
	if (lines.size() < p_line) {
		sentry::util::print_error("Script source is smaller than the referenced line, lineno: ", p_line);
		return true;
	}

	r_context_line = lines[p_line - 1];
	r_pre_context = lines.slice(MAX(p_line - 6, 0), p_line - 1);
	r_post_context = lines.slice(p_line, MIN(p_line + 5, lines.size()));
	return false;
}

} // unnamed namespace

void SentryLogger::_log_error(const String &p_function, const String &p_file, int32_t p_line,
		const String &p_code, const String &p_rationale, bool p_editor_notify, int32_t p_error_type,
		const TypedArray<ScriptBacktrace> &p_script_backtraces) {
	Ref<SentryLoggerLimits> limits = SentryOptions::get_singleton()->get_logger_limits();
	bool as_breadcrumb = SentryOptions::get_singleton()->should_capture_breadcrumb((GodotErrorType)p_error_type);
	bool as_event = SentryOptions::get_singleton()->should_capture_event((GodotErrorType)p_error_type) &&
			// frame_events < limits->events_per_frame &&
			event_times.size() < limits->throttle_events;

	if (!as_breadcrumb && !as_event) {
		// Bail out if capture is disabled for this error type.
		return;
	}

	// Debug output.
	if (SentryOptions::get_singleton()->is_debug_enabled()) {
		sentry::util::print_debug(
				"Error logged:\n",
				"   Function: \"", p_function, "\"\n",
				"   File: ", p_file, "\n",
				"   Line: ", p_line, "\n",
				"   Code: ", p_code, "\n",
				"   Rationale: ", p_rationale, "\n",
				"   Error Type: ", error_types[int(p_error_type)]);
	}

	TimePoint now = std::chrono::high_resolution_clock::now();

	// Capture error as event.
	if (as_event) {
		Vector<sentry::InternalSDK::StackFrame> frames;

		// Backtraces don't include variables by default, so if we need them, we must capture them separately.
		bool include_variables = SentryOptions::get_singleton()->is_logger_include_variables_enabled();
		TypedArray<ScriptBacktrace> script_backtraces = include_variables ? Engine::get_singleton()->capture_script_backtraces(true) : p_script_backtraces;

		// Select script backtrace with the biggest number of frames (best-effort heuristic).
		// Why: We don't know the order of frames across all backtraces (only within each one),
		// so we must pick one.
		int64_t selected_index = -1;
		int64_t selected_num_frames = -1;
		for (int i = 0; i < script_backtraces.size(); i++) {
			const Ref<ScriptBacktrace> backtrace = script_backtraces[i];
			if (backtrace->get_frame_count() > selected_num_frames) {
				selected_index = i;
				selected_num_frames = backtrace->get_frame_count();
			}
		}

		if (selected_index >= 0) {
			const Ref<ScriptBacktrace> backtrace = script_backtraces[selected_index];
			String platform = backtrace->get_language_name().to_lower().remove_char(' ');
			for (int frame_idx = backtrace->get_frame_count() - 1; frame_idx >= 0; frame_idx--) {
				sentry::InternalSDK::StackFrame stack_frame{
					backtrace->get_frame_file(frame_idx),
					backtrace->get_frame_function(frame_idx),
					backtrace->get_frame_line(frame_idx),
					true, // in_app
					platform
				};

				// Provide script source code context for script errors if available.
				if (SentryOptions::get_singleton()->is_logger_include_source_enabled()) {
					// Provide script source code context for script errors if available.
					String context_line;
					PackedStringArray pre_context;
					PackedStringArray post_context;
					bool err = _get_script_context(backtrace->get_frame_file(frame_idx),
							backtrace->get_frame_line(frame_idx), context_line, pre_context, post_context);
					if (!err) {
						stack_frame.context_line = context_line;
						stack_frame.pre_context = pre_context;
						stack_frame.post_context = post_context;
					}
				}

				// Variables.
				if (include_variables) {
					int32_t num_locals = backtrace->get_local_variable_count(frame_idx);
					int32_t num_members = backtrace->get_member_variable_count(frame_idx);
					int32_t num_globals = backtrace->get_global_variable_count();
					int32_t num_vars = num_locals + num_members + num_globals;
					int32_t starting_index = 0;

					stack_frame.vars.resize(num_vars);

					for (int i = 0; i < num_locals; i++) {
						stack_frame.vars.set(starting_index + i,
								Pair(backtrace->get_local_variable_name(frame_idx, i), backtrace->get_local_variable_value(frame_idx, i)));
					}
					starting_index += num_locals;

					for (int i = 0; i < num_members; i++) {
						stack_frame.vars.set(starting_index + i,
								Pair(backtrace->get_member_variable_name(frame_idx, i), backtrace->get_member_variable_value(frame_idx, i)));
					}
					starting_index += num_members;

					for (int i = 0; i < num_globals; i++) {
						stack_frame.vars.set(starting_index + i,
								Pair(backtrace->get_global_variable_name(i), backtrace->get_global_variable_value(i)));
					}
				}

				frames.append(stack_frame);
			}
		}

		if (p_error_type == ErrorType::ERROR_TYPE_ERROR) {
			// Add native frame to the top so it is preserved as the source of error.
			frames.append({ p_file, p_function, p_line, false, "native" });
		}

		SentrySDK::get_singleton()->get_internal_sdk()->capture_error(
				error_types[int(p_error_type)],
				p_rationale.is_empty() ? p_code : p_rationale,
				sentry::get_sentry_level_for_godot_error_type((GodotErrorType)p_error_type),
				frames);

		// For throttling
		// frame_events++;
		event_times.push_back(now);
	}

	// Capture error as breadcrumb.
	if (as_breadcrumb) {
		Dictionary data;
		data["function"] = p_function;
		data["file"] = p_file;
		data["line"] = p_line;
		data["code"] = p_code;
		data["rationale"] = p_rationale;
		data["error_type"] = String(error_types[int(p_error_type)]);

		SentrySDK::get_singleton()->add_breadcrumb(
				p_rationale,
				"error",
				sentry::get_sentry_level_for_godot_error_type((GodotErrorType)p_error_type),
				"error",
				data);
	}
}

void SentryLogger::_log_message(const String &p_message, bool p_error) {
	std::string std_message{ p_message.ascii() };

	// Patterns that are checked against each message (like Sentry debug printing).
	for (auto pattern : filters) {
		if (std::regex_search(std_message, pattern)) {
			return;
		}
	}

	// Godot prints three lines like these enclosing 2 tracebacks -- native and script.
	// We filter all that output. One of these lines starts with a linebreak -- therefore 2 pattern checks.
	if (p_message.begins_with(LINE_WITH_EQUAL_SIGNS_STARTER) ||
			p_message.begins_with(LINE_WITH_EQUAL_SIGNS)) {
		num_lines_with_equal_signs++;
		if (num_lines_with_equal_signs > 2) {
			num_lines_with_equal_signs = 0;
			return;
		}
	}

	bool is_printing_backtrace = (num_lines_with_equal_signs > 0);
	if (is_printing_backtrace) {
		// Don't log backtrace printing.
		return;
	}

	SentrySDK::get_singleton()->add_breadcrumb(
			p_message,
			"log",
			p_error ? sentry::Level::LEVEL_ERROR : sentry::Level::LEVEL_INFO,
			"debug");
}

SentryLogger::SentryLogger() {
	LINE_WITH_EQUAL_SIGNS_STARTER = "\n================================================================";
	LINE_WITH_EQUAL_SIGNS = "================================================================";

	filters = {
		// Filter Sentry messages
		std::regex{ "^[A-Z]+: Sentry:" },
	};
}
