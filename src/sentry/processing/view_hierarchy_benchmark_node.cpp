#include "view_hierarchy_benchmark_node.h"

#include "view_hierarchy.h"
#include "view_hierarchy_builder.h"

#include <chrono>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace sentry {

void ViewHierarchyBenchmarkNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start_benchmark"), &ViewHierarchyBenchmarkNode::start_benchmark);
	ClassDB::bind_method(D_METHOD("stop_benchmark"), &ViewHierarchyBenchmarkNode::stop_benchmark);
	ClassDB::bind_method(D_METHOD("is_benchmark_running"), &ViewHierarchyBenchmarkNode::is_benchmark_running);
}

ViewHierarchyBenchmarkNode::ViewHierarchyBenchmarkNode() {
	builder = memnew(ViewHierarchyBuilder);
	set_process(false); // Start with processing disabled
}

ViewHierarchyBenchmarkNode::~ViewHierarchyBenchmarkNode() {
	if (builder) {
		memdelete(builder);
		builder = nullptr;
	}
}

void ViewHierarchyBenchmarkNode::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_ready();
			break;
		}
		case NOTIFICATION_PROCESS: {
			double delta = get_process_delta_time();
			_process(delta);
			break;
		}
	}
}

void ViewHierarchyBenchmarkNode::_ready() {
	print_line("ViewHierarchyBenchmarkNode: Auto-starting benchmark...");
	initialize_benchmark();
	start_benchmark();
}

void ViewHierarchyBenchmarkNode::_process(double p_delta) {
	if (!benchmark_active) {
		return;
	}

	process_frame();
}

void ViewHierarchyBenchmarkNode::initialize_benchmark() {
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

	original_root = Object::cast_to<Node>(root_window);
	if (!original_root) {
		print_line("ERROR: Could not cast root to Node for benchmarking");
		return;
	}

	// Reset all state
	current_phase = PHASE_SETUP;
	current_iteration = 0;
	original_total_time = 0.0;
	optimized_total_time = 0.0;
	original_large_total_time = 0.0;
	optimized_large_total_time = 0.0;
	test_hierarchy_created = false;
	total_test_nodes = 0;

	// Auto-start will be handled in _ready()
}

void ViewHierarchyBenchmarkNode::start_benchmark() {
	if (benchmark_active) {
		return;
	}

	print_line("\n=== VIEW HIERARCHY PERFORMANCE BENCHMARK (FRAME-BASED) ===");
	print_line(String("Running ") + String::num_int64(total_iterations) + String(" iterations per phase..."));

	benchmark_active = true;
	current_phase = PHASE_SETUP;
	current_iteration = 0;
	set_process(true);
}

void ViewHierarchyBenchmarkNode::stop_benchmark() {
	if (!benchmark_active) {
		return;
	}

	benchmark_active = false;
	set_process(false);

	if (test_hierarchy_created) {
		cleanup_test_hierarchy();
	}

	print_line("Benchmark stopped.");
}

bool ViewHierarchyBenchmarkNode::is_benchmark_running() const {
	return benchmark_active;
}

void ViewHierarchyBenchmarkNode::process_frame() {
	if (!benchmark_active) {
		return;
	}

	switch (current_phase) {
		case PHASE_SETUP: {
			print_line("=== TESTING WITH ORIGINAL SMALL HIERARCHY ===");
			advance_to_next_phase();
			break;
		}

		case PHASE_SMALL_ORIGINAL: {
			if (current_iteration == 0) {
				print_line("\n--- Original Implementation ---");
			}

			// Sample once per frame
			frame_start_time = std::chrono::high_resolution_clock::now();
			String result = build_view_hierarchy_json();
			auto current_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - frame_start_time);
			double elapsed_us = static_cast<double>(duration.count());

			original_total_time += elapsed_us;
			print_line(String("Frame ") + String::num_int64(current_iteration + 1) + String("/") + String::num_int64(total_iterations) +
					String(" - Original: ") + String::num_int64(elapsed_us) + String(" us, ") +
					String::num_int64(result.length()) + String(" chars"));

			current_iteration++;
			if (current_iteration >= total_iterations) {
				advance_to_next_phase();
			}
			break;
		}

		case PHASE_SMALL_OPTIMIZED: {
			if (current_iteration == 0) {
				print_line("\n--- Optimized Implementation ---");
			}

			// Sample once per frame
			frame_start_time = std::chrono::high_resolution_clock::now();
			auto buffer = builder->build_json();
			auto current_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - frame_start_time);
			double elapsed_us = static_cast<double>(duration.count());

			optimized_total_time += elapsed_us;
			print_line(String("Frame ") + String::num_int64(current_iteration + 1) + String("/") + String::num_int64(total_iterations) +
					String(" - Optimized: ") + String::num_int64(elapsed_us) + String(" us, ") +
					String::num_int64(buffer.get_used()) + String(" chars"));

			current_iteration++;
			if (current_iteration >= total_iterations) {
				// Present small hierarchy results
				print_line("\n=== SMALL HIERARCHY PERFORMANCE COMPARISON ===");
				double avg_original = original_total_time / total_iterations;
				double avg_optimized = optimized_total_time / total_iterations;

				print_line(String("Average time (original): ") + String::num(avg_original, 2) + String(" us"));
				print_line(String("Average time (optimized): ") + String::num(avg_optimized, 2) + String(" us"));

				double optimized_vs_original = avg_original / avg_optimized;
				print_line(String("Optimized vs Original: ") + String::num(optimized_vs_original, 2) + String("x"));

				if (optimized_vs_original > 1.1) {
					print_line(String::utf8("✓ Optimized method is faster than original"));
				} else if (optimized_vs_original < 0.9) {
					print_line(String::utf8("⚠ Optimized method is slower than original"));
				} else {
					print_line(String::utf8("≈ Optimized and original methods perform similarly"));
				}

				advance_to_next_phase();
			}
			break;
		}

		case PHASE_LARGE_SETUP: {
			print_line("\n=== CREATING LARGE TEST HIERARCHY ===");
			create_large_test_hierarchy();
			print_line(String("Added ") + String::num_int64(total_test_nodes) + String(" test nodes to hierarchy"));
			print_line("\n=== TESTING WITH LARGE HIERARCHY ===");
			advance_to_next_phase();
			break;
		}

		case PHASE_LARGE_ORIGINAL: {
			if (current_iteration == 0) {
				print_line("\n--- Original Implementation (Large Hierarchy) ---");
			}

			// Sample once per frame
			frame_start_time = std::chrono::high_resolution_clock::now();
			String result = build_view_hierarchy_json();
			auto current_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - frame_start_time);
			double elapsed_us = static_cast<double>(duration.count());

			original_large_total_time += elapsed_us;
			print_line(String("Frame ") + String::num_int64(current_iteration + 1) + String("/") + String::num_int64(total_iterations) +
					String(" - Original (large): ") + String::num_int64(elapsed_us) + String(" us, ") +
					String::num_int64(result.length()) + String(" chars"));

			current_iteration++;
			if (current_iteration >= total_iterations) {
				advance_to_next_phase();
			}
			break;
		}

		case PHASE_LARGE_OPTIMIZED: {
			if (current_iteration == 0) {
				print_line("\n--- Optimized Implementation (Large Hierarchy) ---");
			}

			// Sample once per frame
			frame_start_time = std::chrono::high_resolution_clock::now();
			auto buffer = builder->build_json();
			auto current_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - frame_start_time);
			double elapsed_us = static_cast<double>(duration.count());

			optimized_large_total_time += elapsed_us;
			print_line(String("Frame ") + String::num_int64(current_iteration + 1) + String("/") + String::num_int64(total_iterations) +
					String(" - Optimized (large): ") + String::num_int64(elapsed_us) + String(" us, ") +
					String::num_int64(buffer.get_used()) + String(" chars"));

			current_iteration++;
			if (current_iteration >= total_iterations) {
				advance_to_next_phase();
			}
			break;
		}

		case PHASE_CLEANUP: {
			// cleanup_test_hierarchy();
			present_results();
			advance_to_next_phase();
			break;
		}

		case PHASE_COMPLETE: {
			benchmark_active = false;
			set_process(false);
			print_line("=== BENCHMARK COMPLETE - NODE AUTO-TERMINATING ===");
			// Auto-remove from scene tree
			get_parent()->remove_child(this);
			queue_free();
			break;
		}
	}
}

void ViewHierarchyBenchmarkNode::advance_to_next_phase() {
	current_iteration = 0;

	switch (current_phase) {
		case PHASE_SETUP:
			// current_phase = PHASE_SMALL_ORIGINAL;
			current_phase = PHASE_SMALL_OPTIMIZED;
			break;
		case PHASE_SMALL_ORIGINAL:
			// current_phase = PHASE_SMALL_OPTIMIZED;
			current_phase = PHASE_LARGE_SETUP;
			break;
		case PHASE_SMALL_OPTIMIZED:
			// current_phase = PHASE_LARGE_SETUP;
			current_phase = PHASE_SMALL_ORIGINAL;
			break;
		case PHASE_LARGE_SETUP:
			// current_phase = PHASE_LARGE_ORIGINAL;
			current_phase = PHASE_LARGE_OPTIMIZED;
			break;
		case PHASE_LARGE_ORIGINAL:
			// current_phase = PHASE_LARGE_OPTIMIZED;
			current_phase = PHASE_CLEANUP;
			break;
		case PHASE_LARGE_OPTIMIZED:
			// current_phase = PHASE_CLEANUP;
			current_phase = PHASE_LARGE_ORIGINAL;
			break;
		case PHASE_CLEANUP:
			current_phase = PHASE_COMPLETE;
			break;
		case PHASE_COMPLETE:
			// Already complete
			break;
	}
}

void ViewHierarchyBenchmarkNode::create_large_test_hierarchy() {
	if (!original_root || test_hierarchy_created) {
		return;
	}

	total_test_nodes = 0;
	create_test_hierarchy_recursive(original_root, 5, 6, total_test_nodes);
	test_hierarchy_created = true;
}

Node *ViewHierarchyBenchmarkNode::create_test_hierarchy_recursive(Node *parent, int depth, int children_per_node, int &total_nodes) {
	if (depth <= 0 || !parent) {
		return nullptr;
	}

	for (int i = 0; i < children_per_node; i++) {
		Node *child = memnew(Node);
		child->set_name(String("TestNode_") + String::num_int64(depth) + String("_") + String::num_int64(i));
		parent->add_child(child);
		total_nodes++;

		// Add some with scene paths occasionally
		if (i % 3 == 0) {
			child->set_scene_file_path(String("res://test_scenes/node_") + String::num_int64(i) + String(".tscn"));
		}

		// Recursively create children
		create_test_hierarchy_recursive(child, depth - 1, children_per_node, total_nodes);
	}

	return parent;
}

void ViewHierarchyBenchmarkNode::cleanup_test_hierarchy() {
	if (!test_hierarchy_created || !original_root) {
		return;
	}

	print_line("\n=== CLEANING UP TEST HIERARCHY ===");
	cleanup_test_hierarchy_recursive(original_root);
	test_hierarchy_created = false;
	total_test_nodes = 0;
	print_line("Test hierarchy cleaned up successfully");
}

void ViewHierarchyBenchmarkNode::cleanup_test_hierarchy_recursive(Node *root) {
	if (!root) {
		return;
	}

	// Remove all test children (those with "TestNode_" prefix)
	TypedArray<Node> children_to_remove;
	for (int i = 0; i < root->get_child_count(); i++) {
		Node *child = root->get_child(i);
		if (child->get_name().begins_with("TestNode_")) {
			children_to_remove.push_back(child);
		} else {
			// Recursively clean non-test nodes
			cleanup_test_hierarchy_recursive(child);
		}
	}

	// Remove and delete test nodes
	for (int i = 0; i < children_to_remove.size(); i++) {
		Node *child = Object::cast_to<Node>(children_to_remove[i]);
		if (child) {
			cleanup_test_hierarchy_recursive(child);
			root->remove_child(child);
			child->queue_free();
		}
	}
}

void ViewHierarchyBenchmarkNode::present_results() {
	print_line("\n=== LARGE HIERARCHY PERFORMANCE COMPARISON ===");
	double avg_original_large = original_large_total_time / total_iterations;
	double avg_optimized_large = optimized_large_total_time / total_iterations;

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

	print_line("\n=== FINAL BENCHMARK SUMMARY ===");
	double avg_original_small = original_total_time / total_iterations;
	double avg_optimized_small = optimized_total_time / total_iterations;

	print_line(String("Small hierarchy - Original: ") + String::num(avg_original_small, 2) + String(" us"));
	print_line(String("Small hierarchy - Optimized: ") + String::num(avg_optimized_small, 2) + String(" us"));
	print_line(String("Large hierarchy - Original: ") + String::num(avg_original_large, 1) + String(" us"));
	print_line(String("Large hierarchy - Optimized: ") + String::num(avg_optimized_large, 1) + String(" us"));

	double small_ratio = avg_original_small / avg_optimized_small;
	double large_ratio = avg_original_large / avg_optimized_large;

	print_line(String("Performance improvement (small): ") + String::num(small_ratio, 2) + String("x"));
	print_line(String("Performance improvement (large): ") + String::num(large_ratio, 2) + String("x"));
}

} // namespace sentry
