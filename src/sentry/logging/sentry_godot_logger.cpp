#include "sentry_godot_logger.h"

#include "sentry/logging/print.h"
#include "sentry/logging/state.h"
#include "sentry/sentry_options.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <string_view>

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
		sentry::logging::print_error("Failed to load script ", p_file);
		return false;
	}

	String source_code = script->get_source_code();
	if (source_code.is_empty()) {
		sentry::logging::print_debug("Script source not available ", p_file.utf8().ptr());
		return false;
	}

	PackedStringArray lines = script->get_source_code().split("\n");
	if (lines.size() < p_line) {
		sentry::logging::print_error("Script source is smaller than the referenced line, lineno: ", p_line);
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

template <class T>
inline size_t _hash(const T &p_value) {
	std::hash<T> hasher;
	return hasher(p_value);
}

template <class T>
inline void _hash_combine(std::size_t &p_hash, const T &p_value) {
	// NOTE: Hash combining technique, originally from boost.
	std::hash<T> hasher;
	p_hash ^= hasher(p_value) + 0x9e3779b9 + (p_hash << 6) + (p_hash >> 2);
}

String _strip_invisible(const String &p_text) {
	String result;

	int i = 0;
	int length = p_text.length();

	while (i < length) {
		char32_t c = p_text[i];

		// Detect ANSI escape sequences: ESC (0x1B) + '['
		if (c == 0x1B && i + 1 < length && p_text[i + 1] == '[') {
			i += 2;
			// Skip until we reach a final byte (0x40-0x7E) aka [A-Za-z0-9].
			while (i < length) {
				char32_t cc = p_text[i];
				if (cc >= 0x40 && cc <= 0x7E) {
					i++;
					break;
				}
				i++;
			}
			continue;
		}

		// Skip control characters (ASCII < 0x20 or DEL 0x7F)
		if (c < 0x20 || c == 0x7F) {
			i++;
			continue;
		}

		result += c;
		i++;
	}

	return result;
}

} // unnamed namespace

namespace sentry::logging {

std::size_t SentryGodotLogger::ErrorKeyHash::operator()(const ErrorKey &p_key) const {
	CharString message_cstr = p_key.message.utf8();
	CharString filename_cstr = p_key.file.utf8();

	std::string_view message_sv{ message_cstr.get_data() };
	std::string_view filename_sv{ filename_cstr.get_data() };

	size_t hash_value = _hash(message_sv);
	_hash_combine(hash_value, filename_sv);
	_hash_combine(hash_value, p_key.line);
	return hash_value;
}

void SentryGodotLogger::_connect_process_frame() {
	MainLoop *main_loop = Engine::get_singleton()->get_main_loop();
	ERR_FAIL_NULL_MSG(main_loop, "SentryGodotLogger: Failed to connect to \"process_frame\" signal - main loop is null.");
	SceneTree *scene_tree = Object::cast_to<SceneTree>(main_loop);
	ERR_FAIL_NULL_MSG(scene_tree, "SentryGodotLogger: Failed to connect to \"process_frame\" signal - expected SceneTree instance as main loop.");

	Callable callable = callable_mp(this, &SentryGodotLogger::_process_frame);
	if (!scene_tree->is_connected("process_frame", callable)) {
		scene_tree->connect("process_frame", callable);
	}
}

void SentryGodotLogger::_disconnect_process_frame() {
	SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	Callable callable = callable_mp(this, &SentryGodotLogger::_process_frame);
	if (scene_tree && scene_tree->is_connected("process_frame", callable)) {
		scene_tree->disconnect("process_frame", callable);
	}
}

void SentryGodotLogger::_process_frame() {
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
	if (unlikely(error_timepoints.size() > 100)) {
		error_timepoints.clear();
	}
}

void SentryGodotLogger::_log_error(const String &p_function, const String &p_file, int32_t p_line,
		const String &p_code, const String &p_rationale, bool p_editor_notify, int32_t p_error_type,
		const TypedArray<Ref<ScriptBacktrace>> &p_script_backtraces) {
	static thread_local uint32_t num_entries = 0;
	constexpr uint32_t MAX_ENTRIES = 5;
	RecursionGuard feedback_loop_guard{ &num_entries, MAX_ENTRIES };
	if (!feedback_loop_guard.can_enter()) {
		ERR_PRINT_ONCE("SentryGodotLogger::_log_error() feedback loop detected.");
		return;
	}

	String error_message = p_rationale.is_empty() ? p_code : p_rationale;
	String error_type = error_type_as_string[int(p_error_type)];

	ErrorKey error_key;
	error_key.message = error_message;
	error_key.file = p_file;
	error_key.line = p_line;

	TimePoint now = std::chrono::high_resolution_clock::now();

	bool as_event = false;
	bool as_breadcrumb = false;
	bool as_log = false;

	{
		std::lock_guard lock{ error_mutex };

		// Reject errors based on per-source-line throttling window to prevent
		// repetitive logging caused by loops or errors recurring in each frame.
		// The timestamps are tracked for each source line that produced an error.
		auto it = error_timepoints.find(error_key);
		bool is_spammy_error = it != error_timepoints.end() && now - it->second < limits.repeated_error_window;

		bool within_frame_limit = frame_events < limits.events_per_frame;
		bool within_throttling_limit = event_times.size() < limits.throttle_events || limits.throttle_window.count() == 0;

		as_event = SentryOptions::get_singleton()->should_capture_event((GodotErrorType)p_error_type) &&
				within_frame_limit &&
				within_throttling_limit &&
				!is_spammy_error;
		as_breadcrumb = SentryOptions::get_singleton()->should_capture_breadcrumb((GodotErrorType)p_error_type) &&
				!is_spammy_error;
		as_log = SentryOptions::get_singleton()->get_experimental()->get_enable_logs() &&
				!is_spammy_error;

		if (as_event) {
			// We decided to capture the error as event (it's happening).
			frame_events++;
			event_times.push_back(now);
		}

		if (as_event || as_breadcrumb) {
			// Store timestamp to prevent repetitive logging from the same line of code.
			error_timepoints[error_key] = now;
		}
	}

	if (!as_breadcrumb && !as_event && !as_log) {
		sentry::logging::print_debug("error capture skipped due to limits");
		return;
	}

	sentry::logging::print_debug(
			"Capturing error: ", error_message,
			"\n   at: ", p_function, " (", p_file, ":", p_line, ")",
			"\n   event: ", as_event, "  breadcrumb: ", as_breadcrumb, "  log: ", as_log);

	String event_uuid;

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
			error_type,
			error_message,
			frames
		};
		ev->add_exception(exception);
		ev->set_logger(logger_name);
		event_uuid = SentrySDK::get_singleton()->capture_event(ev);
	}

	// Capture error as breadcrumb.
	if (as_breadcrumb) {
		Dictionary data;
		data["function"] = p_function;
		data["file"] = p_file;
		data["line"] = p_line;
		data["code"] = p_code;
		data["rationale"] = p_rationale;
		data["error_type"] = error_type;

		Ref<SentryBreadcrumb> crumb = SentryBreadcrumb::create(error_message);
		crumb->set_level(sentry::get_sentry_level_for_godot_error_type((GodotErrorType)p_error_type));
		crumb->set_type("error");
		crumb->set_category("error");
		crumb->set_data(data);
		SentrySDK::get_singleton()->add_breadcrumb(crumb);
	}

	// Capture as structured log.
	if (as_log) {
		String body = vformat("%s: %s\n   at: %s (%s:%d)",
				error_type,
				error_message,
				p_function,
				p_file,
				p_line);
		if (as_event) {
			// TODO: Should just leave it as attribute?
			body += "\n   event_id: " + event_uuid;
		}

		LogLevel log_level = sentry::get_sentry_log_level_for_godot_error_type((GodotErrorType)p_error_type);

		Dictionary attributes;
		attributes["sentry.event_id"] = event_uuid;
		attributes["error.function"] = p_function;
		attributes["error.file"] = p_file;
		attributes["error.line"] = p_line;
		attributes["error.type"] = error_type;
		if (!p_code.is_empty()) {
			attributes["error.code"] = p_code;
		}
		if (!p_rationale.is_empty()) {
			attributes["error.rationale"] = p_rationale;
		}

		SentrySDK::get_singleton()->get_internal_sdk()->log(log_level, body, attributes);
	}
}

void SentryGodotLogger::_log_message(const String &p_message, bool p_error) {
	sentry::logging::MessageScope message_scope;

	bool as_log = SentryOptions::get_singleton()->get_experimental()->get_enable_logs();
	bool as_breadcrumb = SentryOptions::get_singleton()->is_logger_messages_as_breadcrumbs_enabled();

	if (!as_log && !as_breadcrumb) {
		return;
	}

	static thread_local uint32_t num_entries = 0;
	constexpr uint32_t MAX_ENTRIES = 5;
	RecursionGuard feedback_loop_guard{ &num_entries, MAX_ENTRIES };
	if (!feedback_loop_guard.can_enter()) {
		ERR_PRINT_ONCE("SentryGodotLogger::_log_message() feedback loop detected.");
		return;
	}

	String processed_message = _strip_invisible(p_message);

	if (processed_message.is_empty()) {
		// Don't add empty breadcrumb.
		return;
	}

	// Filtering: Check message prefixes to skip certain messages (e.g., Sentry's own debug output).
	for (const String &prefix : filter_by_prefix) {
		if (processed_message.begins_with(prefix)) {
			return;
		}
	}

	if (as_log) {
		sentry::LogLevel level = p_error ? LOG_LEVEL_ERROR : LOG_LEVEL_INFO;
		SentrySDK::get_singleton()->get_internal_sdk()->log(level, processed_message, log_attributes);
	}

	if (as_breadcrumb) {
		sentry::Level level = p_error ? LEVEL_ERROR : LEVEL_INFO;
		Ref<SentryBreadcrumb> crumb = SentryBreadcrumb::create(processed_message);
		crumb->set_category("log");
		crumb->set_level(level);
		crumb->set_type("debug");
		SentrySDK::get_singleton()->add_breadcrumb(crumb);
	}
}

void SentryGodotLogger::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_connect_process_frame"), &SentryGodotLogger::_connect_process_frame);
}

void SentryGodotLogger::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			SceneTree *scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
			if (scene_tree) {
				_connect_process_frame();
			} else {
				// Defer signal connection since SceneTree is not available during early initialization.
				call_deferred("_connect_process_frame");
			}
		} break;
		case NOTIFICATION_PREDELETE: {
			_disconnect_process_frame();
		} break;
	}
}

SentryGodotLogger::SentryGodotLogger() {
	logger_name = "SentryGodotLogger";

	// TODO: Update according to spec.
	log_attributes["sentry.origin"] = "auto.godot.logger";

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

SentryGodotLogger::~SentryGodotLogger() {
	if (!Engine::get_singleton()) {
		return;
	}
}

} //namespace sentry::logging
