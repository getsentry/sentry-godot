#include "auto_metrics_integration.h"

#include "sentry/auto_metrics/sentry_auto_metrics.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace sentry {

namespace {

void start_collection(uint64_t p_runner_id) {
	SentryAutoMetrics *runner = Object::cast_to<SentryAutoMetrics>(ObjectDB::get_instance(p_runner_id));
	if (runner == nullptr || runner->is_queued_for_deletion()) {
		return;
	}

	if (!SENTRY_OPTIONS()->get_auto_metrics()->is_any_enabled()) {
		return;
	}

	SceneTree *tree = SceneTree::get_singleton();
	ERR_FAIL_NULL_MSG(tree, "Sentry: Failed to enable auto metrics - SceneTree not available.");
	Window *root = tree->get_root();
	ERR_FAIL_NULL_MSG(root, "Sentry: Failed to enable auto metrics - root window not available.");
	root->add_child(runner, false, Node::INTERNAL_MODE_BACK);
}

} //namespace

bool AutoMetricsIntegration::setup(const Ref<SentryOptions> &p_options) {
	if (!p_options->get_auto_metrics()->is_any_enabled()) {
		return false;
	}

	SentryAutoMetrics *runner = memnew(SentryAutoMetrics);
	_runner_id = runner->get_instance_id();
	callable_mp_static(&start_collection).bind(_runner_id.operator uint64_t()).call_deferred();
	return true;
}

void AutoMetricsIntegration::teardown() {
	SentryAutoMetrics *runner = Object::cast_to<SentryAutoMetrics>(ObjectDB::get_instance(_runner_id));
	_runner_id = ObjectID();
	if (runner == nullptr) {
		return;
	}

	if (runner->is_inside_tree()) {
		runner->get_parent()->remove_child(runner);
		runner->queue_free();
	} else {
		memdelete(runner);
	}
}

} //namespace sentry
