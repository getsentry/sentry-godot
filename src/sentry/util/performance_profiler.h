#pragma once

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::util {

struct PhaseStats {
	String name;
	uint64_t total_time_usec = 0;
	uint32_t run_count = 0;
	uint64_t min_time_usec = UINT64_MAX;
	uint64_t max_time_usec = 0;
	uint64_t last_time_usec = 0;
	int32_t nesting_level = 0;
	int32_t first_encounter_order = -1;

	double get_average_usec() const {
		return run_count > 0 ? static_cast<double>(total_time_usec) / run_count : 0.0;
	}

	double get_average_msec() const {
		return get_average_usec() / 1000.0;
	}

	double get_last_msec() const {
		return static_cast<double>(last_time_usec) / 1000.0;
	}
};

class PerformanceProfiler {
private:
	HashMap<String, PhaseStats> phase_stats;
	Vector<String> phase_stack;
	Vector<uint64_t> start_times;
	Vector<String> phase_order;
	int32_t next_order_id = 0;
	bool enabled = true;

public:
	// Convenience method to time a phase automatically via RAII
	class PhaseTimer {
	private:
		PerformanceProfiler *profiler;
		String phase_name;

	public:
		PhaseTimer(PerformanceProfiler *p_profiler, const String &p_phase_name);
		~PhaseTimer();
	};

	PerformanceProfiler() = default;
	~PerformanceProfiler() = default;

	// Enable/disable profiling
	void set_enabled(bool p_enabled) { enabled = p_enabled; }
	bool is_enabled() const { return enabled; }

	// Start timing a phase
	void start_phase(const String &p_phase_name);

	// End timing the current phase and record the duration
	void end_phase();

	// Create a phase timer (RAII style)
	PhaseTimer time_phase(const String &p_phase_name);

	// Get stats for a specific phase
	const PhaseStats *get_phase_stats(const String &p_phase_name) const;

	// Get all phase names
	Vector<String> get_phase_names() const;

	// Reset stats for a phase
	void reset_phase(const String &p_phase_name);

	// Reset all stats
	void reset_all();

	// Print stats for all phases
	void print_stats() const;

	// Print stats for a specific phase
	void print_phase_stats(const String &p_phase_name) const;

	// Export stats as dictionary (for debugging/serialization)
	Dictionary export_stats() const;
};

// Macro for easy phase timing
#define SENTRY_PROFILE_PHASE(profiler, phase_name) \
	auto _timer = (profiler).time_phase(phase_name)

} //namespace sentry::util
