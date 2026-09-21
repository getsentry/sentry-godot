// Integration tests verifying that .NET options.Native.SetBeforeSendTransaction callbacks
// are invoked only for native transactions and can mutate or discard them.

#if defined(TESTS_ENABLED)

#include "cpp_test_helpers.h"
#include "dotnet_test_support.h"

#include "sentry/dotnet/csharp_interop.h"
#include "sentry/processing/process_event.h"
#include "sentry/processing/process_transaction.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;
using namespace sentry;

using sentry::tests::InitFixture;

namespace {

// The pipeline receives transactions as SentryEvent: a regular event is sufficient
// to test the managed bridge.
Ref<SentryEvent> _make_dummy_event() {
	return SentrySDK::get_singleton()->create_event();
}

} // unnamed namespace

TEST_SUITE("[.NET] Test options.Native.SetBeforeSendTransaction bridging") {
	TEST_CASE("Processing pipeline forwards native transactions to the managed callback") {
		if (!sentry::dotnet::godot_supports_dotnet()) {
			MESSAGE("Skipping: managed runtime unavailable (non-mono Godot build).");
			return;
		}

		InitFixture fixture("InitWithNativeHooks"); // inits the SDK, closes at scope exit
		REQUIRED_CHECK(fixture.get_harness() != nullptr);
		REQUIRED_CHECK(sentry::dotnet::is_managed_layer_registered());
		REQUIRED_CHECK(sentry::dotnet::is_before_send_defined());
		REQUIRED_CHECK(sentry::dotnet::is_before_send_transaction_defined());

		SUBCASE("Transaction passes through the callback") {
			Ref<SentryEvent> transaction = _make_dummy_event();
			transaction->set_release("before-release@1.0.0");
			transaction->set_dist("before-distribution");
			transaction->set_environment("before-environment");
			transaction->set_tag("before_send_transaction.read_me", "read-value");
			transaction->set_tag("before_send_transaction.remove_me", "remove-value");

			Ref<SentryEvent> result = sentry::process_transaction(transaction);
			REQUIRED_CHECK(result.is_valid());

			Dictionary seen = fixture.get_harness()->call("GetSeenTransactionValues");
			CHECK(seen["release"] == "before-release@1.0.0");
			CHECK(seen["distribution"] == "before-distribution");
			CHECK(seen["environment"] == "before-environment");
			CHECK(seen["tag"] == "read-value");

			CHECK(transaction->get_release() == "after-release@2.0.0");
			CHECK(transaction->get_dist() == "after-distribution");
			CHECK(transaction->get_environment() == "after-environment");
			CHECK(transaction->get_tag("before_send_transaction.added") == "added-value");
			CHECK(transaction->get_tag("before_send_transaction.remove_me").is_empty());
		}

		SUBCASE("Transaction can be discarded") {
			Ref<SentryEvent> transaction = _make_dummy_event();
			transaction->set_tag("before_send_transaction.drop", "true");
			CHECK(sentry::process_transaction(transaction).is_null());
		}

		SUBCASE("Transaction invokes only the transaction callback") {
			const int64_t event_calls_before = fixture.get_harness()->call("GetNativeBeforeSendCallCount");
			const int64_t transaction_calls_before = fixture.get_harness()->call("GetNativeBeforeSendTransactionCallCount");
			sentry::process_transaction(_make_dummy_event());
			const int64_t event_calls_after = fixture.get_harness()->call("GetNativeBeforeSendCallCount");
			const int64_t transaction_calls_after = fixture.get_harness()->call("GetNativeBeforeSendTransactionCallCount");
			CHECK(event_calls_after == event_calls_before);
			CHECK(transaction_calls_after == transaction_calls_before + 1);
		}

		SUBCASE("Event does not invoke the transaction callback") {
			const int64_t calls_before = fixture.get_harness()->call("GetNativeBeforeSendTransactionCallCount");
			sentry::process_event(SentrySDK::get_singleton()->create_event());
			const int64_t calls_after = fixture.get_harness()->call("GetNativeBeforeSendTransactionCallCount");
			CHECK(calls_after == calls_before);
		}
	}
}

#endif // TESTS_ENABLED
