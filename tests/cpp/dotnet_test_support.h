#pragma once

#ifdef TESTS_ENABLED

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace sentry::tests {

// Loads the .NET test harness once and returns it, or null on a non-mono build.
godot::Object *get_dotnet_harness();

// Scoped fixture for [.NET] suites.
// Initializes the SDK via the given harness method, and closes the session at scope exit.
class InitFixture {
private:
	godot::Object *harness = nullptr;

public:
	godot::Object *get_harness() const { return harness; }

	explicit InitFixture(const godot::StringName &p_init_method) :
			harness(get_dotnet_harness()) {
		if (harness != nullptr) {
			harness->call(p_init_method);
		}
	}

	~InitFixture() {
		if (harness != nullptr) {
			harness->call("Close");
		}
	}
};

} // namespace sentry::tests

#endif // TESTS_ENABLED
