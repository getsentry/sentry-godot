#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/platform/platform.h"

TEST_SUITE("Platform queries") {
	TEST_CASE("Queries current process resident memory on supported platforms") {
		uint64_t bytes = 42;
#if defined(WINDOWS_ENABLED) || defined(LINUX_ENABLED) || defined(ANDROID_ENABLED) || defined(MACOS_ENABLED) || defined(IOS_ENABLED)
		REQUIRED_CHECK(sentry::platform::get_process_resident_memory(bytes));
		CHECK(bytes > 0);
#else
		CHECK_FALSE(sentry::platform::get_process_resident_memory(bytes));
		CHECK(bytes == 42);
#endif
	}
}

#endif // TESTS_ENABLED
