#ifdef TESTS_ENABLED

#include "cpp_test_runner.h"

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry::tests {

constexpr const char *CLI_TEST_FLAG = "--test-sentry";

bool should_run_cpp_tests() {
	return OS::get_singleton()->get_cmdline_args().has(CLI_TEST_FLAG);
}

[[noreturn]] void run_cpp_tests_and_exit() {
	UtilityFunctions::print("Starting doctest session...");

	doctest::Context context;

	// Forward args after `--test-sentry` to doctest (e.g. --test-case="*pattern*", --no-colors).
	// Use `godot --headless --path <PROJECT> --test-sentry --help` for info.
	PackedStringArray cli_args = OS::get_singleton()->get_cmdline_args();
	Vector<CharString> forwarded_args;
	forwarded_args.push_back("sentry-cpp-tests"); // synthetic binary name
	int idx = cli_args.find(CLI_TEST_FLAG);
	if (idx >= 0) {
		for (int i = idx + 1; i < cli_args.size(); ++i) {
			forwarded_args.push_back(cli_args[i].utf8());
		}
	}
	Vector<const char *> argv;
	for (const CharString &arg : forwarded_args) {
		argv.push_back(arg.get_data());
	}
	context.applyCommandLine(static_cast<int>(argv.size()), argv.ptr());

	const int result = context.run();

	UtilityFunctions::print("Exit code: " + String::num_int64(result));

	// Exit without cleanup -- there is nothing to tear down before the process ends.
	std::_Exit(result);
}

} // namespace sentry::tests

#endif // TESTS_ENABLED
