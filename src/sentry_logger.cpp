#include "sentry_logger.h"

#include "sentry/internal_sdk.h"
#include "sentry/util/print.h"
#include "sentry_options.h"
#include "sentry_sdk.h"

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

		// Select script backtrace with the biggest number of frames (best-effort heuristic).
		// Why: We can't reliably identify the backtrace that led to an error in the C++/native code,
		// and we don't know the order of frames across all backtraces (only within each one).
		int64_t selected_index = -1;
		int64_t selected_num_frames = -1;
		for (int i = 0; i < p_script_backtraces.size(); i++) {
			const Ref<ScriptBacktrace> backtrace = p_script_backtraces[i];
			if (backtrace->get_frame_count() > selected_num_frames) {
				selected_index = i;
				selected_num_frames = backtrace->get_frame_count();
			}
		}

		if (selected_index >= 0) {
			const Ref<ScriptBacktrace> backtrace = p_script_backtraces[selected_index];
			String platform = backtrace->get_language_name().to_lower().remove_char(' ');
			for (int i = backtrace->get_frame_count() - 1; i >= 0; i--) {
				sentry::InternalSDK::StackFrame stack_frame{
					backtrace->get_frame_file(i),
					backtrace->get_frame_function(i),
					backtrace->get_frame_line(i),
					true, // in_app
					platform
				};

				// Provide script source code context for script errors if available.
				if (SentryOptions::get_singleton()->is_logger_include_source_enabled()) {
					// Provide script source code context for script errors if available.
					String context_line;
					PackedStringArray pre_context;
					PackedStringArray post_context;
					bool err = _get_script_context(backtrace->get_frame_file(i), backtrace->get_frame_line(i), context_line, pre_context, post_context);
					if (!err) {
						stack_frame.context_line = context_line;
						stack_frame.pre_context = pre_context;
						stack_frame.post_context = post_context;
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
		data["function"] = String(p_function);
		data["file"] = String(p_file);
		data["line"] = p_line;
		data["code"] = p_code;
		data["godot_error_type"] = String(error_types[int(p_error_type)]);

		SentrySDK::get_singleton()->add_breadcrumb(
				p_rationale,
				"error",
				sentry::get_sentry_level_for_godot_error_type((GodotErrorType)p_error_type),
				"error",
				data);
	}
}

void SentryLogger::_log_message(const String &p_message, bool p_error) {
	if (p_message.contains(" Sentry: ")) {
		// Don't log Sentry messages.
		return;
	}

	SentrySDK::get_singleton()->add_breadcrumb(
			p_message,
			"log",
			p_error ? sentry::Level::LEVEL_ERROR : sentry::Level::LEVEL_INFO,
			"debug");
}
