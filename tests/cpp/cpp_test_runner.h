#pragma once

#ifdef TESTS_ENABLED

namespace sentry::tests {

bool should_run_cpp_tests();

[[noreturn]] void run_cpp_tests_and_exit();

} // namespace sentry::tests

#endif // TESTS_ENABLED
