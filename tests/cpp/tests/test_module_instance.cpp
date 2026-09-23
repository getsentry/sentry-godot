#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/util/module_instance.h"

#include <godot_cpp/variant/string_name.hpp>

using namespace godot;
using sentry::util::ModuleInstance;

namespace {

struct TrackedData {
	inline static int constructions = 0;
	inline static int destructions = 0;

	const StringName name{ "module_instance_test" };

	TrackedData() { ++constructions; }
	~TrackedData() { ++destructions; }
};

} // namespace

TEST_SUITE("ModuleInstance") {
	TEST_CASE("Repeated creation reuses the same instance") {
		using Data = TrackedData;
		using Instance = ModuleInstance<Data>;
		const int constructions = Data::constructions;
		const int destructions = Data::destructions;
		Instance::create_once();
		const auto *instance = &Instance::get();
		Instance::create_once();
		CHECK(&Instance::get() == instance);
		CHECK(Data::constructions == constructions + 1);
		CHECK(String(instance->name) == "module_instance_test");
		CHECK(Data::destructions == destructions);
	}
}

#endif // TESTS_ENABLED
