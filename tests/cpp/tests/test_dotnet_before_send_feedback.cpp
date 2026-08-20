// Integration tests verifying that .NET options.Native.SetBeforeSendFeedback callbacks
// are invoked for user feedback captured outside .NET and can mutate or discard it.

#if defined(TESTS_ENABLED)

#include "cpp_test_helpers.h"
#include "dotnet_test_support.h"

#include "sentry/dotnet/csharp_interop.h"
#include "sentry/processing/process_event.h"
#include "sentry/processing/process_feedback.h"
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

TEST_SUITE("[.NET] Test options.Native.SetBeforeSendFeedback bridging") {
	TEST_CASE("Feedback processing forwards user feedback to the managed callback") {
		if (!sentry::dotnet::godot_supports_dotnet()) {
			MESSAGE("Skipping: managed runtime unavailable (non-mono Godot build).");
			return;
		}

		InitFixture fixture("InitWithNativeHooks"); // inits the SDK, closes at scope exit
		REQUIRE(fixture.get_harness() != nullptr);
		REQUIRE(sentry::dotnet::is_managed_layer_registered());
		REQUIRE(sentry::dotnet::is_before_send_feedback_defined());

		SUBCASE("Feedback passes through the callback") {
			// Create feedback carrying known values.
			// The callback reads them through getters, and then overrides.
			Ref<SentryEvent> event = _make_message_event("Feedback (should be kept)");
			event->set_tag("before_send_feedback.read_me", "read-value");

			Ref<SentryEvent> result = sentry::process_feedback(event);
			REQUIRE(result.is_valid());

			SUBCASE("Getters observe the values set on the feedback") {
				Dictionary seen = fixture.get_harness()->call("GetSeenFeedbackValues");
				CHECK(seen["message"] == "Feedback (should be kept)");
				CHECK(seen["tag"] == "read-value");
			}

			SUBCASE("Setters mutate the feedback in place") {
				CHECK(event->get_level() == sentry::LEVEL_WARNING);
				CHECK(event->get_tag("before_send_feedback.added") == String::utf8("added 世界 👋"));
			}
		}

		SUBCASE("DROP_ME feedback is discarded") {
			// The callback discards it, so process_feedback returns null.
			Ref<SentryEvent> drop_event = _make_message_event("DROP_ME feedback");
			Ref<SentryEvent> drop_result = sentry::process_feedback(drop_event);
			CHECK(drop_result.is_null());
		}

		SUBCASE("Feedback invokes the callback exactly once") {
			const int64_t calls_before = fixture.get_harness()->call("GetNativeBeforeSendFeedbackCallCount");
			sentry::process_feedback(_make_message_event("Counted feedback"));
			const int64_t calls_after = fixture.get_harness()->call("GetNativeBeforeSendFeedbackCallCount");
			CHECK(calls_after == calls_before + 1);
		}

		SUBCASE("Feedback never reaches the before-send callback") {
			const int64_t calls_before = fixture.get_harness()->call("GetNativeBeforeSendCallCount");
			sentry::process_feedback(_make_message_event("Feedback stays out of before-send"));
			const int64_t calls_after = fixture.get_harness()->call("GetNativeBeforeSendCallCount");
			CHECK(calls_after == calls_before);
		}

		SUBCASE("Events never reach the before-send-feedback callback") {
			const int64_t calls_before = fixture.get_harness()->call("GetNativeBeforeSendFeedbackCallCount");
			sentry::process_event(_make_message_event("Event stays out of before-send-feedback"));
			const int64_t calls_after = fixture.get_harness()->call("GetNativeBeforeSendFeedbackCallCount");
			CHECK(calls_after == calls_before);
		}
	}
}

#endif // TESTS_ENABLED
