#pragma once

#include <chrono>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>

namespace sentry {

class ViewHierarchyBuilder; // Forward declaration

// Auto-starting, frame-based benchmark node that:
// - Initializes and starts benchmark automatically in NOTIFICATION_READY
// - Samples one iteration per frame in NOTIFICATION_PROCESS (to avoid CPU cache optimizations)
// - Presents results when complete and auto-removes itself from the scene
class ViewHierarchyBenchmarkNode : public godot::Node {
	GDCLASS(ViewHierarchyBenchmarkNode, godot::Node);

private:
	// Benchmark state
	enum BenchmarkPhase {
		PHASE_SETUP,
		PHASE_SMALL_ORIGINAL,
		PHASE_SMALL_OPTIMIZED,
		PHASE_LARGE_SETUP,
		PHASE_LARGE_ORIGINAL,
		PHASE_LARGE_OPTIMIZED,
		PHASE_CLEANUP,
		PHASE_COMPLETE
	};

	BenchmarkPhase current_phase = PHASE_SETUP;
	int current_iteration = 0;
	int total_iterations = 50;
	bool benchmark_active = false;
	bool test_hierarchy_created = false;
	int total_test_nodes = 0;

	// Performance tracking
	double original_total_time = 0.0;
	double optimized_total_time = 0.0;
	double original_large_total_time = 0.0;
	double optimized_large_total_time = 0.0;

	std::chrono::high_resolution_clock::time_point frame_start_time;

	ViewHierarchyBuilder *builder = nullptr;
	godot::Node *original_root = nullptr;

	// Helper methods
	void initialize_benchmark();
	void process_frame();
	void run_single_iteration();
	void advance_to_next_phase();
	void create_large_test_hierarchy();
	void cleanup_test_hierarchy();
	void present_results();
	godot::Node *create_test_hierarchy_recursive(godot::Node *parent, int depth, int children_per_node, int &total_nodes);
	void cleanup_test_hierarchy_recursive(godot::Node *root);

protected:
	static void _bind_methods();

public:
	ViewHierarchyBenchmarkNode();
	~ViewHierarchyBenchmarkNode();

	void _notification(int p_what);
	void _ready() override;
	void _process(double p_delta) override;

	// Public interface (mainly for monitoring - node auto-starts and auto-ends)
	void start_benchmark();
	void stop_benchmark();
	bool is_benchmark_running() const;
};

} // namespace sentry
