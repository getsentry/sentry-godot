// Integration tests for .NET options.Native.SetBeforeSend bridged to the native layer.

#if defined(TESTS_ENABLED)

#include "cpp_test_helpers.h"
#include "dotnet_test_support.h"

#include "sentry/dotnet/csharp_interop.h"
#include "sentry/processing/process_event.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;
using namespace sentry;

using sentry::tests::InitFixture;

namespace {

Ref<SentryEvent> _make_message_event(const String &p_message) {
	Ref<SentryEvent> event = SentrySDK::get_singleton()->create_event();
	event->set_message(p_message);
	return event;
}

} // unnamed namespace

TEST_SUITE("[.NET] Test options.Native.SetBeforeSend bridging") {
	TEST_CASE("Processing pipeline forwards native events to the managed callback") {
		if (!sentry::dotnet::godot_supports_dotnet()) {
			MESSAGE("Skipping: managed runtime unavailable (non-mono Godot build).");
			return;
		}

		InitFixture fixture("InitWithNativeHooks"); // inits the SDK, closes at scope exit
		REQUIRE(fixture.get_harness() != nullptr);
		REQUIRE(sentry::dotnet::is_managed_layer_registered());
		REQUIRE(sentry::dotnet::is_before_send_defined());

		SUBCASE("Event passes through the callback") {
			// Create event carrying know values.
			// The callback reads them through getters, and then overrides.
			Ref<SentryEvent> event = _make_message_event("Native event (should be kept)");
			event->set_level(sentry::LEVEL_ERROR);
			event->set_release("read-release@1.2.3");
			event->set_environment("read-environment");
			event->set_logger("read-logger");
			event->set_tag("before_send.read_me", "read-value");
			event->set_tag("before_send.remove_me", "remove-me");

			Ref<SentryEvent> result = sentry::process_event(event);
			REQUIRE(result.is_valid());

			SUBCASE("Getters observe the values set on the event") {
				Dictionary seen = fixture.get_harness()->call("GetSeenEventValues");
				CHECK(seen["message"] == "Native event (should be kept)");
				CHECK(seen["level"] == "Error");
				CHECK(seen["release"] == "read-release@1.2.3");
				CHECK(seen["environment"] == "read-environment");
				CHECK(seen["logger"] == "read-logger");
				CHECK(seen["tag"] == "read-value");
			}

			SUBCASE("Setters mutate the event in place") {
				CHECK(event->get_message() == String::utf8("Before-send override: 世界 👋"));
				CHECK(event->get_level() == sentry::LEVEL_WARNING);
				CHECK(event->get_tag("before_send.added") == String::utf8("added 世界 👋"));
				CHECK(event->get_tag("before_send.remove_me").is_empty());
			}
		}

		SUBCASE("DROP_ME event is discarded") {
			// The callback discards it, so process_event returns null.
			Ref<SentryEvent> drop_event = _make_message_event("DROP_ME native event");
			Ref<SentryEvent> drop_result = sentry::process_event(drop_event);
			CHECK(drop_result.is_null());
		}
	}
}

#endif // TESTS_ENABLED
