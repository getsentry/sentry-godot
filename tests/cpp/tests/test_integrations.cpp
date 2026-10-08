#if defined(TESTS_ENABLED) && defined(SDK_NATIVE)

#include "cpp_test_helpers.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <vector>

using namespace godot;
using namespace sentry;

namespace sentry {

class IntegrationTestAccess {
public:
	static void add(SentrySDK *p_sdk, SentryIntegration *p_integration) {
		p_sdk->_add_integration(p_integration, p_sdk->get_options());
	}
};

} //namespace sentry

namespace {

struct IntegrationObserver {
	std::vector<String> calls;
	Ref<SentryOptions> setup_options;
	bool backend_enabled_during_teardown = true;
};

class SpyIntegration final : public SentryIntegration {
	SENTRY_CASTABLE(SpyIntegration, SentryIntegration);

private:
	IntegrationObserver &_observer;
	const char *_name;
	bool _setup_result;

public:
	SpyIntegration(IntegrationObserver &p_observer, const char *p_name, bool p_setup_result = true) :
			_observer(p_observer), _name(p_name), _setup_result(p_setup_result) {}

	const char *get_name() const override { return _name; }

	bool setup(const Ref<SentryOptions> &p_options) override {
		_observer.calls.push_back(String("setup: ") + _name);
		_observer.setup_options = p_options;
		return _setup_result;
	}

	void teardown() override {
		_observer.calls.push_back(String("teardown: ") + _name);
		_observer.backend_enabled_during_teardown &= SentrySDK::get_singleton()->is_enabled();
	}

	~SpyIntegration() override {
		_observer.calls.push_back(String("destroy: ") + _name);
	}
};

Ref<SentryEvent> _discard_event(const Ref<SentryEvent> &) {
	return {};
}

void _configure_sdk(const Ref<SentryOptions> &p_options) {
	p_options->set_dsn("https://public@127.0.0.1/1");
	p_options->set_debug_enabled(false);
	p_options->set_attach_log(false);
	p_options->set_attach_screenshot(false);
	p_options->set_shutdown_timeout_ms(0);
	p_options->set_before_send(callable_mp_static(&_discard_event));
	p_options->get_godot_logger()->set_enabled(false);
}

class InitFixture {
private:
	SentrySDK *_sdk = SentrySDK::get_singleton();

public:
	InitFixture() {
		_sdk->init(callable_mp_static(&_configure_sdk));
	}

	~InitFixture() { _sdk->close(); }

	SentrySDK *get_sdk() const { return _sdk; }
};

} //namespace

TEST_SUITE("Integration lifecycle") {
	TEST_CASE("Close tears down integrations in reverse order before closing the backend") {
		IntegrationObserver observer;
		InitFixture fixture;
		SentrySDK *sdk = fixture.get_sdk();
		REQUIRED_CHECK(sdk->is_enabled());
		IntegrationTestAccess::add(sdk, memnew(SpyIntegration(observer, "first")));
		IntegrationTestAccess::add(sdk, memnew(SpyIntegration(observer, "second")));
		CHECK(observer.setup_options == sdk->get_options());
		CHECK(observer.calls == std::vector<String>({ "setup: first", "setup: second" }));

		sdk->close();
		const std::vector<String> expected = {
			"setup: first",
			"setup: second",
			"teardown: second",
			"destroy: second",
			"teardown: first",
			"destroy: first",
		};
		CHECK(observer.calls == expected);
		CHECK(observer.backend_enabled_during_teardown);
		CHECK_FALSE(sdk->is_enabled());

		sdk->close();
		CHECK(observer.calls == expected);
	}

	TEST_CASE("Failed setup tears down and destroys the integration immediately") {
		IntegrationObserver observer;
		InitFixture fixture;
		SentrySDK *sdk = fixture.get_sdk();
		REQUIRED_CHECK(sdk->is_enabled());
		IntegrationTestAccess::add(sdk, memnew(SpyIntegration(observer, "failed", false)));
		const std::vector<String> expected = { "setup: failed", "teardown: failed", "destroy: failed" };
		CHECK(observer.calls == expected);
		CHECK(observer.setup_options == sdk->get_options());
		CHECK(observer.backend_enabled_during_teardown);
		CHECK(sdk->is_enabled());

		sdk->close();
		CHECK(observer.calls == expected);
	}

	TEST_CASE("Failed setup preserves other installed integrations") {
		IntegrationObserver observer;
		InitFixture fixture;
		SentrySDK *sdk = fixture.get_sdk();
		REQUIRED_CHECK(sdk->is_enabled());
		IntegrationTestAccess::add(sdk, memnew(SpyIntegration(observer, "first")));
		IntegrationTestAccess::add(sdk, memnew(SpyIntegration(observer, "failed", false)));
		IntegrationTestAccess::add(sdk, memnew(SpyIntegration(observer, "second")));
		const std::vector<String> after_setup = {
			"setup: first",
			"setup: failed",
			"teardown: failed",
			"destroy: failed",
			"setup: second",
		};
		CHECK(observer.calls == after_setup);

		sdk->close();
		const std::vector<String> after_close = {
			"setup: first",
			"setup: failed",
			"teardown: failed",
			"destroy: failed",
			"setup: second",
			"teardown: second",
			"destroy: second",
			"teardown: first",
			"destroy: first",
		};
		CHECK(observer.calls == after_close);
		CHECK(observer.backend_enabled_during_teardown);
	}

	TEST_CASE("Reinitialized SDK manages a new integration cycle with fresh options") {
		IntegrationObserver observer;
		Ref<SentryOptions> first_options;
		{
			InitFixture fixture;
			SentrySDK *sdk = fixture.get_sdk();
			REQUIRED_CHECK(sdk->is_enabled());
			IntegrationTestAccess::add(sdk, memnew(SpyIntegration(observer, "first")));
			first_options = observer.setup_options;
		}

		{
			InitFixture fixture;
			SentrySDK *sdk = fixture.get_sdk();
			REQUIRED_CHECK(sdk->is_enabled());
			IntegrationTestAccess::add(sdk, memnew(SpyIntegration(observer, "second")));
			CHECK(observer.setup_options == sdk->get_options());
			CHECK(observer.setup_options != first_options);
		}

		const std::vector<String> expected = {
			"setup: first",
			"teardown: first",
			"destroy: first",
			"setup: second",
			"teardown: second",
			"destroy: second",
		};
		CHECK(observer.calls == expected);
	}
}

#endif // TESTS_ENABLED && SDK_NATIVE
