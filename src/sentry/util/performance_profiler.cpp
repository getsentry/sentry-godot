#include "performance_profiler.h"

#include "sentry/util/print.h"

#include <godot_cpp/classes/time.hpp>

namespace sentry::util {

void PerformanceProfiler::start_phase(const String &p_phase_name) {
	if (!enabled) {
		return;
	}

	uint64_t current_time = Time::get_singleton()->get_ticks_usec();
	phase_stack.push_back(p_phase_name);
	start_times.push_back(current_time);

	// Track nesting level and order
	int32_t current_nesting = phase_stack.size() - 1;

	if (!phase_stats.has(p_phase_name)) {
		// First time encountering this phase
		PhaseStats &stats = phase_stats[p_phase_name];
		stats.name = p_phase_name;
		stats.nesting_level = current_nesting;
		stats.first_encounter_order = next_order_id++;
		phase_order.push_back(p_phase_name);
	}
}

void PerformanceProfiler::end_phase() {
	if (!enabled) {
		return;
	}

	if (phase_stack.is_empty()) {
		sentry::util::print_error("PerformanceProfiler: Trying to end phase but no phase is currently active");
		return;
	}

	uint64_t end_time = Time::get_singleton()->get_ticks_usec();

	String phase_name = phase_stack[phase_stack.size() - 1];
	uint64_t start_time = start_times[start_times.size() - 1];
	uint64_t duration = end_time - start_time;

	// Remove from stacks
	phase_stack.remove_at(phase_stack.size() - 1);
	start_times.remove_at(start_times.size() - 1);

	// Update stats
	PhaseStats &stats = phase_stats[phase_name];
	stats.total_time_usec += duration;
	stats.run_count++;
	stats.last_time_usec = duration;

	if (duration < stats.min_time_usec) {
		stats.min_time_usec = duration;
	}

	if (duration > stats.max_time_usec) {
		stats.max_time_usec = duration;
	}
}

PerformanceProfiler::PhaseTimer::PhaseTimer(PerformanceProfiler *p_profiler, const String &p_phase_name) :
		profiler(p_profiler), phase_name(p_phase_name) {
	if (profiler) {
		profiler->start_phase(phase_name);
	}
}

PerformanceProfiler::PhaseTimer::~PhaseTimer() {
	if (profiler) {
		profiler->end_phase();
	}
}

PerformanceProfiler::PhaseTimer PerformanceProfiler::time_phase(const String &p_phase_name) {
	return PhaseTimer(this, p_phase_name);
}

const PhaseStats *PerformanceProfiler::get_phase_stats(const String &p_phase_name) const {
	if (phase_stats.has(p_phase_name)) {
		return &phase_stats[p_phase_name];
	}
	return nullptr;
}

Vector<String> PerformanceProfiler::get_phase_names() const {
	Vector<String> names;
	for (const KeyValue<String, PhaseStats> &entry : phase_stats) {
		names.push_back(entry.key);
	}
	return names;
}

void PerformanceProfiler::reset_phase(const String &p_phase_name) {
	phase_stats.erase(p_phase_name);
}

void PerformanceProfiler::reset_all() {
	phase_stats.clear();
	phase_stack.clear();
	start_times.clear();
	phase_order.clear();
	next_order_id = 0;
}

void PerformanceProfiler::print_stats() const {
	if (phase_stats.is_empty()) {
		sentry::util::print_debug("PerformanceProfiler: No phase data available");
		return;
	}

	String output = "Performance stats:";

	// Print phases in order they were first encountered
	for (const String &phase_name : phase_order) {
		if (phase_stats.has(phase_name)) {
			const PhaseStats &stats = phase_stats[phase_name];

			// Create indentation based on nesting level
			String indent = "";
			for (int32_t i = 0; i < stats.nesting_level; i++) {
				indent += "  ";
			}

			output += vformat("\n%s  Phase \"%s\": runs=%d, avg=%.3fms, last=%.3fms, min=%.3fms, max=%.3fms",
					indent,
					stats.name,
					stats.run_count,
					stats.get_average_msec(),
					stats.get_last_msec(),
					stats.min_time_usec / 1000.0,
					stats.max_time_usec / 1000.0);
		}
	}

	sentry::util::print_debug(output);
}

void PerformanceProfiler::print_phase_stats(const String &p_phase_name) const {
	const PhaseStats *stats = get_phase_stats(p_phase_name);
	if (!stats) {
		sentry::util::print_debug("PerformanceProfiler: No data for phase '", p_phase_name, "'");
		return;
	}

	sentry::util::print_debug(
			"Phase '", stats->name, "': ",
			"runs=", stats->run_count,
			", avg=", String::num(stats->get_average_msec(), 3), "ms",
			", last=", String::num(stats->get_last_msec(), 3), "ms",
			", min=", String::num(static_cast<double>(stats->min_time_usec) / 1000.0, 3), "ms",
			", max=", String::num(static_cast<double>(stats->max_time_usec) / 1000.0, 3), "ms");
}

Dictionary PerformanceProfiler::export_stats() const {
	Dictionary result;

	for (const KeyValue<String, PhaseStats> &entry : phase_stats) {
		const PhaseStats &stats = entry.value;
		Dictionary phase_data;

		phase_data["name"] = stats.name;
		phase_data["total_time_usec"] = static_cast<int64_t>(stats.total_time_usec);
		phase_data["run_count"] = static_cast<int32_t>(stats.run_count);
		phase_data["min_time_usec"] = static_cast<int64_t>(stats.min_time_usec);
		phase_data["max_time_usec"] = static_cast<int64_t>(stats.max_time_usec);
		phase_data["last_time_usec"] = static_cast<int64_t>(stats.last_time_usec);
		phase_data["average_usec"] = stats.get_average_usec();
		phase_data["average_msec"] = stats.get_average_msec();

		result[stats.name] = phase_data;
	}

	return result;
}

} //namespace sentry::util
