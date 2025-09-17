#include "view_hierarchy_benchmark.h"

#include "view_hierarchy.h"
#include "view_hierarchy_builder.h"

#include <chrono>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace {

// Performance measurement utility
class PerformanceTimer {
private:
	std::chrono::high_resolution_clock::time_point start_time;
	String operation_name;
	bool print_on_destruction;

public:
	PerformanceTimer(const String &name, bool auto_print = false) :
			operation_name(name), print_on_destruction(auto_print) {
		start_time = std::chrono::high_resolution_clock::now();
	}

	~PerformanceTimer() {
		if (print_on_destruction) {
			print_performance();
		}
	}

	double get_elapsed_microseconds() const {
		auto current_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time);
		return static_cast<double>(duration.count());
	}

	double get_elapsed_milliseconds() const {
		return get_elapsed_microseconds() / 1000.0;
	}

	void print_performance() const {
		double microseconds = get_elapsed_microseconds();
		print_line(operation_name + String(" took: ") + String::num_int64(microseconds) + String(" us"));
	}
};

// Create a large test hierarchy for benchmarking
Node *create_test_hierarchy(Node *parent, int depth, int children_per_node, int &total_nodes) {
	if (depth <= 0)
		return nullptr;

	for (int i = 0; i < children_per_node; i++) {
		Node *child = memnew(Node);
		child->set_name(String("TestNode_") + String::num_int64(depth) + String("_") + String::num_int64(i));
		parent->add_child(child);
		total_nodes++;

		// Add some with scripts and scene paths occasionally
		if (i % 3 == 0) {
			// Simulate having a scene file path
			child->set_scene_file_path(String("res://test_scenes/node_") + String::num_int64(i) + String(".tscn"));
		}

		// Recursively create children
		create_test_hierarchy(child, depth - 1, children_per_node, total_nodes);
	}

	return parent;
}

// Clean up test hierarchy
void cleanup_test_hierarchy(Node *root) {
	if (!root)
		return;

	// Remove all children recursively
	while (root->get_child_count() > 0) {
		Node *child = root->get_child(0);
		root->remove_child(child);
		cleanup_test_hierarchy(child);
		memdelete(child);
	}
}

} // anonymous namespace

namespace sentry {

// Performance comparison function
void benchmark_view_hierarchy_performance() {
	print_line("\n=== VIEW HIERARCHY PERFORMANCE BENCHMARK ===");

	SceneTree *sml = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	if (!sml) {
		print_line("ERROR: Could not get SceneTree for benchmarking");
		return;
	}

	Window *root_window = sml->get_root();
	if (!root_window) {
		print_line("ERROR: Could not get root window for benchmarking");
		return;
	}

	Node *original_root = Object::cast_to<Node>(root_window);
	if (!original_root) {
		print_line("ERROR: Could not cast root to Node for benchmarking");
		return;
	}

	ViewHierarchyBuilder builder;

	// Test with original small hierarchy first
	print_line("=== TESTING WITH ORIGINAL SMALL HIERARCHY ===");

	const int NUM_ITERATIONS = 50;
	double original_total_time = 0;
	double optimized_total_time = 0;

	print_line(String("Running ") + String::num_int64(NUM_ITERATIONS) + String(" iterations of each implementation...\n"));

	print_line("\n--- Original Implementation ---");
	for (int i = 0; i < NUM_ITERATIONS; i++) {
		PerformanceTimer timer(String("Original iteration ") + String::num_int64(i + 1), true);
		String result = build_view_hierarchy_json();
		original_total_time += timer.get_elapsed_microseconds();
		print_line(String("Original result: ") + String::num_int64(result.length()) + String(" chars"));
	}

	print_line("\n--- Optimized Implementation ---");
	for (int i = 0; i < NUM_ITERATIONS; i++) {
		PerformanceTimer timer(String("Optimized iteration ") + String::num_int64(i + 1), true);
		String result = builder.build_json();
		optimized_total_time += timer.get_elapsed_microseconds();
		print_line(String("Optimized result: ") + String::num_int64(result.length()) + String(" chars"));
	}

	// Performance summary
	print_line("\n=== PERFORMANCE COMPARISON RESULTS ===");
	double avg_original = original_total_time / NUM_ITERATIONS;
	double avg_optimized = optimized_total_time / NUM_ITERATIONS;

	print_line(String("Average time (original): ") + String::num(avg_original, 1) + String(" us"));
	print_line(String("Average time (optimized): ") + String::num(avg_optimized, 1) + String(" us"));

	double optimized_vs_original = avg_original / avg_optimized;

	print_line(String("Optimized vs Original: ") + String::num(optimized_vs_original, 2) + String("x"));

	if (optimized_vs_original > 1.1) {
		print_line(String::utf8("✓ Optimized method is faster than original"));
	} else if (optimized_vs_original < 0.9) {
		print_line(String::utf8("⚠ Optimized method is slower that original"));
	} else {
		print_line(String::utf8("≈ Optimized and original methods perform similarly"));
	}

	// Now test with large hierarchy
	print_line("\n=== CREATING LARGE TEST HIERARCHY ===");
	int total_test_nodes = 0;
	create_test_hierarchy(original_root, 5, 6, total_test_nodes); // 4 levels deep, 8 children per node
	print_line(String("Added ") + String::num_int64(total_test_nodes) + String(" test nodes to hierarchy"));

	// Reset timers for large hierarchy test
	original_total_time = 0;
	optimized_total_time = 0;

	print_line("\n=== TESTING WITH LARGE HIERARCHY ===");
	print_line(String("Running ") + String::num_int64(NUM_ITERATIONS) + String(" iterations on large hierarchy...\n"));

	// Benchmark original implementation on large hierarchy
	print_line("\n--- Original Implementation (Large Hierarchy) ---");
	for (int i = 0; i < NUM_ITERATIONS; i++) {
		PerformanceTimer timer(String("Original large iteration ") + String::num_int64(i + 1), true);
		String result = build_view_hierarchy_json();
		print_line("Original JSON length: ", result.length());
		original_total_time += timer.get_elapsed_microseconds();
		print_line(String("Original large result: ") + String::num_int64(result.length()) + String(" chars"));
	}

	// Benchmark optimized implementation on large hierarchy
	print_line("\n--- Optimized Implementation (Large Hierarchy) ---");
	for (int i = 0; i < NUM_ITERATIONS; i++) {
		PerformanceTimer timer(String("Optimized large iteration ") + String::num_int64(i + 1), true);
		String result = builder.build_json();
		print_line("Optimized JSON length: ", result.length());
		optimized_total_time += timer.get_elapsed_microseconds();
		print_line(String("Optimized large result: ") + String::num_int64(result.length()) + String(" chars"));
	}

	// Large hierarchy performance summary
	print_line("\n=== LARGE HIERARCHY PERFORMANCE COMPARISON ===");
	double avg_original_large = original_total_time / NUM_ITERATIONS;
	double avg_optimized_large = optimized_total_time / NUM_ITERATIONS;

	print_line(String("Average time (original): ") + String::num(avg_original_large, 1) + String(" us"));
	print_line(String("Average time (optimized): ") + String::num(avg_optimized_large, 1) + String(" us"));

	double optimized_vs_original_large = avg_original_large / avg_optimized_large;

	print_line(String("Optimized vs original: ") + String::num(optimized_vs_original_large, 2) + String("x"));

	if (optimized_vs_original_large > 1.1) {
		print_line(String::utf8("✓ Optimized method is faster than original for large hierarchies!"));
	} else if (optimized_vs_original_large < 0.9) {
		print_line(String::utf8("⚠ Original method is still faster even for large hierarchies"));
	} else {
		print_line(String::utf8("≈ Optimized and original methods perform similarly even for large hierarchies"));
	}

	// Clean up test nodes
	print_line("\n=== CLEANING UP TEST HIERARCHY ===");
	cleanup_test_hierarchy(original_root);
	print_line("Test hierarchy cleaned up successfully");
}

} // namespace sentry
