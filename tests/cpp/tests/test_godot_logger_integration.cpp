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
	const Ref<SentryGodotLoggerOptions> logger_options = p_options->get_godot_logger();
	logger_options->set_enabled(p_enabled);
	logger_options->set_event_mask(MASK_ERROR);
	logger_options->set_breadcrumb_mask(MASK_NONE);
	logger_options->set_log_mask(MASK_NONE);
}

class InitFixture {
private:
	SentrySDK *_sdk = SentrySDK::get_singleton();

public:
	explicit InitFixture(bool p_logger_enabled) {
		_sdk->init(callable_mp_static(&_configure_logger).bind(p_logger_enabled));
	}

	~InitFixture() { _sdk->close(); }

	SentrySDK *get_sdk() const { return _sdk; }
};

// Find live SentryGodotLogger instances through their process_frame connections.
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

} //namespace

TEST_SUITE("Godot logger integration") {
	TEST_CASE("SDK installs the Godot logger and removes it on close") {
		captured_events.clear();
		InitFixture fixture(true);
		SentrySDK *sdk = fixture.get_sdk();
		REQUIRED_CHECK(sdk->is_enabled());
		const std::vector<uint64_t> logger_ids = _get_logger_ids();
		REQUIRED_CHECK(logger_ids.size() == 1);

		UtilityFunctions::push_error("integration logger test error");
		REQUIRED_CHECK(captured_events.size() == 1);
		CHECK(captured_events.front().contains("integration logger test error"));
		CHECK(captured_events.front().contains("SentryGodotLogger"));

		sdk->close();
		CHECK(_get_logger_ids().empty());
		CHECK(ObjectDB::get_instance(logger_ids.front()) == nullptr);
		UtilityFunctions::push_error("integration logger test error");
		CHECK(captured_events.size() == 1);
	}

	TEST_CASE("Disabled Godot logger declines installation") {
		captured_events.clear();
		InitFixture fixture(false);
		SentrySDK *sdk = fixture.get_sdk();
		REQUIRED_CHECK(sdk->is_enabled());

		GodotLoggerIntegration integration;
		CHECK_FALSE(integration.setup(sdk->get_options()));
		integration.teardown();
		CHECK(_get_logger_ids().empty());
		UtilityFunctions::push_error("integration logger test error");
		CHECK(captured_events.empty());
	}
}

#endif // TESTS_ENABLED && SDK_NATIVE
