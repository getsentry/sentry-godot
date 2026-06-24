// Integration tests verifying that the native layer follows the .NET layer's
// current trace as it changes.

#if defined(TESTS_ENABLED)

#include "cpp_test_helpers.h"
#include "dotnet_test_support.h"

#include "sentry/dotnet/csharp_interop.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/variant/string.hpp>

using namespace godot;
using namespace sentry;

using sentry::tests::InitFixture;

namespace {

String _native_trace_id() {
	return SentrySDK::get_singleton()->get_trace_context().trace_id;
}

} // unnamed namespace

TEST_SUITE("[.NET] Cross-layer trace sync") {
	TEST_CASE("Native trace follows the .NET layer across a transaction lifecycle") {
		if (!sentry::dotnet::godot_supports_dotnet()) {
			MESSAGE("Skipping: managed runtime unavailable (non-mono Godot build).");
			return;
		}

		InitFixture fixture("Init"); // inits the SDK, closes at scope exit
		Object *harness = fixture.get_harness();
		REQUIRE(harness != nullptr);
		REQUIRE(sentry::dotnet::is_managed_layer_registered());

		// Baseline: .NET adopts native's session trace at init, so the layers start aligned.
		const String session_trace = _native_trace_id();
		CHECK_FALSE(session_trace.is_empty());
		CHECK(String(harness->call("GetCurrentTraceId")) == session_trace);

		// Starting a .NET transaction rotates the .NET trace; native must follow it.
		const String tx_trace = harness->call("StartTransaction", "trace-sync-tx", "test");
		CHECK_FALSE(tx_trace.is_empty());
		CHECK(tx_trace != session_trace);
		CHECK(_native_trace_id() == tx_trace);

		// Finishing regenerates the .NET propagation trace; native must follow again.
		const String post_trace = harness->call("FinishTransaction");
		CHECK_FALSE(post_trace.is_empty());
		CHECK(post_trace != tx_trace);
		CHECK(_native_trace_id() == post_trace);
	}
}

#endif // TESTS_ENABLED
