#include "sentry_logger.h"

#include "sentry/sentry_options.h"
#include "sentry/sentry_sdk.h"
#include "sentry/util/print.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>

namespace {

using SentryEvent = sentry::SentryEvent;
using SentryOptions = sentry::SentryOptions;

// Error enum values as strings
const char *error_type_as_string[] = {
	"ERROR",
	"WARNING",
	"SCRIPT ERROR",
	"SHADER ERROR",
};

// RAII-style recursion guard that prevents entering function recursively more
// than `max_entries` times.
class RecursionGuard {
private:
	uint32_t *counter_ptr = nullptr;
	uint32_t max_entries = 0;

public:
	_FORCE_INLINE_ bool can_enter() const { return *counter_ptr <= max_entries; }

	RecursionGuard(uint32_t *p_counter_ptr, uint32_t p_max_entries) :
			counter_ptr(p_counter_ptr), max_entries(p_max_entries) {
		(*counter_ptr)++;
	}

	~RecursionGuard() { (*counter_ptr)--; }
};

bool _get_script_context(const String &p_file, int p_line, String &r_context_line, PackedStringArray &r_pre_context, PackedStringArray &r_post_context) {
	if (p_file.is_empty()) {
		return false;
	}

	Ref<Script> script = ResourceLoader::get_singleton()->load(p_file);

	// ! Note: Script source code context is only automatically provided if GDScript is exported as text (not binary tokens).

	if (script.is_null()) {
		sentry::util::print_error("Failed to load script ", p_file);
		return false;
	}

	String source_code = script->get_source_code();
	if (source_code.is_empty()) {
		sentry::util::print_debug("Script source not available ", p_file.utf8().ptr());
		return false;
	}

	PackedStringArray lines = script->get_source_code().split("\n");
	if (lines.size() < p_line) {
		sentry::util::print_error("Script source is smaller than the referenced line, lineno: ", p_line);
		return false;
	}

	r_context_line = lines[p_line - 1];
	r_pre_context = lines.slice(MAX(p_line - 6, 0), p_line - 1);
	r_post_context = lines.slice(p_line, MIN(p_line + 5, lines.size()));
	return true;
}

Vector<SentryEvent::StackFrame> _extract_error_stack_frames_from_backtraces(
		const TypedArray<ScriptBacktrace> &p_backtraces,
		const String &p_file,
		int p_line,
		bool p_include_variables) {
	Vector<SentryEvent::StackFrame> frames;

	// Prioritize backtrace with the top frame matching the error's file and linenumber.
	// Otherwise, select backtrace with the biggest number of frames (best-effort heuristic).
	// Why: We don't know the order of frames across all backtraces (only within each one),
	// so we must pick one.
	int64_t selected_index = -1;
	int64_t selected_num_frames = -1;
	for (int i = 0; i < p_backtraces.size(); i++) {
		const Ref<ScriptBacktrace> &backtrace = p_backtraces[i];
		int32_t num_frames = backtrace->get_frame_count();
		if (num_frames && backtrace->get_frame_line(0) == p_line && backtrace->get_frame_file(0) == p_file) {
			// Direct match – prioritize this backtrace.
			selected_index = i;
			break;
		}
		if (num_frames > selected_num_frames) {
			selected_index = i;
			selected_num_frames = backtrace->get_frame_count();
		}
	}

	if (selected_index >= 0) {
		const Ref<ScriptBacktrace> &backtrace = p_backtraces[selected_index];
		String platform = backtrace->get_language_name().to_lower().remove_char(' ');
		for (int frame_idx = backtrace->get_frame_count() - 1; frame_idx >= 0; frame_idx--) {
			SentryEvent::StackFrame stack_frame{
				backtrace->get_frame_file(frame_idx),
				backtrace->get_frame_function(frame_idx),
				backtrace->get_frame_line(frame_idx),
				true, // in_app
				platform
			};

			// Provide script source code context for script errors if available.
			if (SentryOptions::get_singleton()->is_logger_include_source_enabled()) {
				String context_line;
				PackedStringArray pre_context;
				PackedStringArray post_context;
				bool success = _get_script_context(backtrace->get_frame_file(frame_idx),
						backtrace->get_frame_line(frame_idx), context_line, pre_context, post_context);
				if (success) {
					stack_frame.context_line = context_line;
					stack_frame.pre_context = pre_context;
					stack_frame.post_context = post_context;
				}
			}

			// Local and member variables.
			if (p_include_variables) {
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

	return frames;
}

} // unnamed namespace

namespace sentry {

void SentryLogger::_connect_process_frame() {
	SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	if (scene_tree) {
		Callable callable = callable_mp(this, &SentryLogger::_process_frame);
		if (!scene_tree->is_connected("process_frame", callable)) {
			scene_tree->connect("process_frame", callable);
		}
	} else {
		ERR_PRINT("Sentry: Failed to connect `process_frame` signal – main loop is null");
	}
}

void SentryLogger::_disconnect_process_frame() {
	SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	Callable callable = callable_mp(this, &SentryLogger::_process_frame);
	if (scene_tree && scene_tree->is_connected("process_frame", callable)) {
		scene_tree->disconnect("process_frame", callable);
	}
}

void SentryLogger::_process_frame() {
	// NOTE: It's important not to push errors from within this function to avoid deadlocks.
	std::lock_guard lock{ error_mutex };

	// Reset per-frame counter.
	frame_events = 0;

	// Throttling: Remove time points outside of the throttling window.
	auto now = std::chrono::high_resolution_clock::now();
	while (event_times.size() && now - event_times.front() >= limits.throttle_window) {
		event_times.pop_front();
	}

	// Clear source_line_times if it's too big. Cheap and efficient.
	if (unlikely(source_line_times.size() > 100)) {
		source_line_times.clear();
	}
}

void SentryLogger::_log_error(const String &p_function, const String &p_file, int32_t p_line,
		const String &p_code, const String &p_rationale, bool p_editor_notify, int32_t p_error_type,
		const TypedArray<Ref<ScriptBacktrace>> &p_script_backtraces) {
	static thread_local uint32_t num_entries = 0;
	constexpr uint32_t MAX_ENTRIES = 5;
	RecursionGuard feedback_loop_guard{ &num_entries, MAX_ENTRIES };
	if (!feedback_loop_guard.can_enter()) {
		ERR_PRINT_ONCE("SentryLogger::_log_error() feedback loop detected.");
		return;
	}

	SourceLine source_line{ p_file.utf8(), p_line };

	TimePoint now = std::chrono::high_resolution_clock::now();

	bool as_event = false;
	bool as_breadcrumb = false;

	{
		std::lock_guard lock{ error_mutex };

		// Reject errors based on per-source-line throttling window to prevent
		// repetitive logging caused by loops or errors recurring in each frame.
		// The timestamps are tracked for each source line that produced an error.
		auto it = source_line_times.find(source_line);
		bool is_spammy_error = it != source_line_times.end() && now - it->second < limits.repeated_error_window;

		bool within_frame_limit = frame_events < limits.events_per_frame;
		bool within_throttling_limit = event_times.size() < limits.throttle_events;

		as_event = SentryOptions::get_singleton()->should_capture_event((GodotErrorType)p_error_type) &&
				within_frame_limit &&
				within_throttling_limit &&
				!is_spammy_error;
		as_breadcrumb = SentryOptions::get_singleton()->should_capture_breadcrumb((GodotErrorType)p_error_type) &&
				!is_spammy_error;

		if (as_event) {
			// We decided to capture the error as event (it's happening).
			frame_events++;
			event_times.push_back(now);
		}

		if (as_event || as_breadcrumb) {
			// Store timestamp to prevent repetitive logging from the same line of code.
			source_line_times[source_line] = now;
		}
	}

	if (!as_breadcrumb && !as_event) {
		sentry::util::print_debug("error capture skipped due to limits");
		return;
	}

	String error_value = p_rationale.is_empty() ? p_code : p_rationale;

	sentry::util::print_debug(
			"Capturing error: ", error_value,
			"\n   at: ", p_function, " (", p_file, ":", p_line, ")",
			"\n   event: ", as_event, "  breadcrumb: ", as_breadcrumb);

	// Capture error as event.
	if (as_event) {
		// Backtraces don't include variables by default, so if we need them, we must capture them separately.
		bool include_variables = SentryOptions::get_singleton()->is_logger_include_variables_enabled();
		TypedArray<ScriptBacktrace> script_backtraces = include_variables ? Engine::get_singleton()->capture_script_backtraces(true) : p_script_backtraces;

		Vector<SentryEvent::StackFrame> frames = _extract_error_stack_frames_from_backtraces(
				script_backtraces, p_file, p_line, include_variables);

		if (p_error_type == ErrorType::ERROR_TYPE_ERROR) {
			// Add native frame to the top so it is preserved as the source of error.
			frames.append({ p_file, p_function, p_line, false, "native" });
		}

		Ref<SentryEvent> ev = SentrySDK::get_singleton()->create_event();
		ev->set_level(sentry::get_sentry_level_for_godot_error_type((GodotErrorType)p_error_type));
		SentryEvent::Exception exception = {
			error_type_as_string[int(p_error_type)],
			error_value,
			frames
		};
		ev->add_exception(exception);
		SentrySDK::get_singleton()->capture_event(ev);
	}

	// Capture error as breadcrumb.
	if (as_breadcrumb) {
		Dictionary data;
		data["function"] = p_function;
		data["file"] = p_file;
		data["line"] = p_line;
		data["code"] = p_code;
		data["rationale"] = p_rationale;
		data["error_type"] = String(error_type_as_string[int(p_error_type)]);

		SentrySDK::get_singleton()->add_breadcrumb(
				error_value,
				"error",
				sentry::get_sentry_level_for_godot_error_type((GodotErrorType)p_error_type),
				"error",
				data);
	}
}

void SentryLogger::_log_message(const String &p_message, bool p_error) {
	static thread_local uint32_t num_entries = 0;
	constexpr uint32_t MAX_ENTRIES = 5;
	RecursionGuard feedback_loop_guard{ &num_entries, MAX_ENTRIES };
	if (!feedback_loop_guard.can_enter()) {
		ERR_PRINT_ONCE("SentryLogger::_log_message() feedback loop detected.");
		return;
	}

	// Filtering: Check message prefixes to skip certain messages (e.g., Sentry's own debug output).
	for (const String &prefix : filter_by_prefix) {
		if (p_message.begins_with(prefix)) {
			return;
		}
	}

	SentrySDK::get_singleton()->add_breadcrumb(
			p_message,
			"log",
			p_error ? sentry::Level::LEVEL_ERROR : sentry::Level::LEVEL_INFO,
			"debug");
}

void SentryLogger::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			callable_mp(this, &SentryLogger::_connect_process_frame).call_deferred();
		} break;
		case NOTIFICATION_PREDELETE: {
			_disconnect_process_frame();
		} break;
	}
}

SentryLogger::SentryLogger() {
	// Filtering setup.
	filter_by_prefix = {
		// Sentry messages
		"Sentry: "
	};

	// Cache limits.
	Ref<SentryLoggerLimits> logger_limits = SentryOptions::get_singleton()->get_logger_limits();
	limits.events_per_frame = logger_limits->events_per_frame;
	limits.repeated_error_window = std::chrono::milliseconds{ logger_limits->repeated_error_window_ms };
	limits.throttle_events = logger_limits->throttle_events;
	limits.throttle_window = std::chrono::milliseconds{ logger_limits->throttle_window_ms };
}

SentryLogger::~SentryLogger() {
	if (!Engine::get_singleton()) {
		return;
	}

	SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	Callable callable = callable_mp(this, &SentryLogger::_process_frame);
	if (scene_tree && scene_tree->is_connected("process_frame", callable)) {
		scene_tree->disconnect("process_frame", callable);
	}
}

} // namespace sentry
