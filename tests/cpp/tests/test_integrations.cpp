#if defined(TESTS_ENABLED) && defined(SDK_NATIVE)

#include "cpp_test_helpers.h"
#include "sentry/integrations/godot_logger/godot_logger_integration.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <vector>

using namespace godot;
using namespace sentry;

namespace {

std::vector<String> captured_events;

Ref<SentryEvent> _capture_event(const Ref<SentryEvent> &p_event) {
	if (!p_event->is_crash()) {
		captured_events.push_back(p_event->to_json());
	}
	return {};
}

void _configure_logger(const Ref<SentryOptions> &p_options, bool p_enabled) {
	p_options->set_dsn("https://public@127.0.0.1/1");
	p_options->set_debug_enabled(false);
	p_options->set_attach_log(false);
	p_options->set_attach_screenshot(false);
	p_options->set_shutdown_timeout_ms(0);
	p_options->set_before_send(callable_mp_static(&_capture_event));
	p_options->get_godot_logger()->set_enabled(p_enabled);
	p_options->get_godot_logger()->set_event_mask(MASK_ERROR);
	p_options->get_godot_logger()->set_breadcrumb_mask(MASK_NONE);
	p_options->get_godot_logger()->set_log_mask(MASK_NONE);
	p_options->get_godot_logger()->get_limits()->set_repeated_error_window_ms(60000);
}

SentrySDK *_init_with_logger_enabled(bool p_logger_enabled) {
	SentrySDK *sdk = SentrySDK::get_singleton();
	sdk->close();
	sdk->init(callable_mp_static(&_configure_logger).bind(p_logger_enabled));
	return sdk;
}

std::vector<uint64_t> _get_logger_ids() {
	std::vector<uint64_t> ids;
	const TypedArray<Dictionary> connections = SceneTree::get_singleton()->get_signal_connection_list("process_frame");
	for (int i = 0; i < connections.size(); ++i) {
		const Dictionary connection = connections[i];
		const Callable callback = connection["callable"];
		Object *target = ObjectDB::get_instance(callback.get_object_id());
		if (Object::cast_to<logging::SentryGodotLogger>(target)) {
			ids.push_back(target->get_instance_id());
		}
	}
	return ids;
}

void _emit_logger_error() {
	UtilityFunctions::push_error("integration logger test error");
}

} //namespace

TEST_SUITE("Godot logger integration") {
	TEST_CASE("Disabled Godot logger declines installation") {
		captured_events.clear();
		SentrySDK *sdk = _init_with_logger_enabled(false);
		REQUIRED_CHECK(sdk->is_enabled());

		GodotLoggerIntegration integration;
		CHECK_FALSE(integration.setup(sdk->get_options()));
		integration.teardown();
		CHECK(_get_logger_ids().empty());
		_emit_logger_error();
		CHECK(captured_events.empty());
		sdk->close();
	}

	TEST_CASE("SDK close removes and destroys the Godot logger") {
		captured_events.clear();
		SentrySDK *sdk = _init_with_logger_enabled(true);
		REQUIRED_CHECK(sdk->is_enabled());
		const std::vector<uint64_t> logger_ids = _get_logger_ids();
		REQUIRED_CHECK(logger_ids.size() == 1);

		_emit_logger_error();
		REQUIRED_CHECK(captured_events.size() == 1);
		CHECK(captured_events.front().contains("integration logger test error"));
		CHECK(captured_events.front().contains("SentryGodotLogger"));

		sdk->close();
		CHECK(_get_logger_ids().empty());
		CHECK(ObjectDB::get_instance(logger_ids.front()) == nullptr);
		_emit_logger_error();
		CHECK(captured_events.size() == 1);
	}

	TEST_CASE("SDK reinitialization creates fresh Godot logger state") {
		captured_events.clear();
		SentrySDK *sdk = _init_with_logger_enabled(true);
		REQUIRED_CHECK(sdk->is_enabled());
		const std::vector<uint64_t> first_ids = _get_logger_ids();
		REQUIRED_CHECK(first_ids.size() == 1);
		_emit_logger_error();
		REQUIRED_CHECK(captured_events.size() == 1);
		sdk->close();

		_init_with_logger_enabled(false);
		REQUIRED_CHECK(sdk->is_enabled());
		CHECK(_get_logger_ids().empty());
		_emit_logger_error();
		CHECK(captured_events.size() == 1);
		sdk->close();

		_init_with_logger_enabled(true);
		REQUIRED_CHECK(sdk->is_enabled());
		const std::vector<uint64_t> second_ids = _get_logger_ids();
		REQUIRED_CHECK(second_ids.size() == 1);
		CHECK(second_ids.front() != first_ids.front());
		_emit_logger_error();
		CHECK(captured_events.size() == 2);
		sdk->close();
	}
}

#endif // TESTS_ENABLED && SDK_NATIVE
