#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/util/callable_cpp.h"

#include <godot_cpp/classes/node.hpp>

using namespace godot;

namespace {

class Target {
	LIFETIME_OBSERVABLE();

	int &_calls;

public:
	explicit Target(int &p_calls) :
			_calls(p_calls) {}

	void invalidate() { _invalidate_lifetime(); }
	void tick() { ++_calls; }
	// A different method with the same signature as tick().
	void tick_twice() { _calls += 2; }
	// A method with a different signature.
	void add(int p_value) { _calls += p_value; }
	// A method with a return value.
	int sum(int p_a, int p_b) {
		++_calls;
		return p_a + p_b;
	}
	// A method with a referenced arg.
	String text(const String &p_prefix) { return p_prefix + String("done"); }
	// Const methods.
	int read(int p_value) const { return p_value + 1; }
	void tick_const() const { ++_calls; }
};

GDExtensionCallErrorType _call_error(const Callable &p_callable, const Variant **p_arguments = nullptr, int p_argcount = 0) {
	Variant value(p_callable);
	Variant result;
	GDExtensionCallError error = {};
	value.callp("call", p_arguments, p_argcount, result, error);
	return error.error;
}

} //namespace

TEST_SUITE("callable_cpp") {
	TEST_CASE("Void methods without arguments return nil") {
		int calls = 0;
		Target target(calls);
		Callable tick = callable_cpp(&target, &Target::tick);
		CHECK(tick.is_valid());
		CHECK(tick.get_object_id() == 0);
		CHECK(tick.get_argument_count() == 0);
		CHECK(tick.call().get_type() == Variant::NIL);
		CHECK(calls == 1);
	}

	TEST_CASE("Arguments can be passed directly or bound") {
		int calls = 0;
		Target target(calls);
		Callable add = callable_cpp(&target, &Target::add);
		CHECK(add.get_argument_count() == 1);
		add.call(3);
		CHECK(calls == 3);
		add.bind(2).call();
		CHECK(calls == 5);
	}

	TEST_CASE("Methods return values") {
		int calls = 0;
		Target target(calls);
		Callable sum = callable_cpp(&target, &Target::sum);
		CHECK(sum.get_argument_count() == 2);
		CHECK(int(sum.call(2, 3)) == 5);
		CHECK(calls == 1);
	}

	TEST_CASE("Methods accept reference arguments") {
		int calls = 0;
		Target target(calls);
		CHECK(String(callable_cpp(&target, &Target::text).call("work ")) == "work done");
	}

	TEST_CASE("Const targets support const methods with and without return values") {
		int calls = 0;
		const Target target(calls);
		CHECK(int(callable_cpp(&target, &Target::read).call(4)) == 5);
		CHECK(callable_cpp(&target, &Target::tick_const).call().get_type() == Variant::NIL);
		CHECK(calls == 1);
	}

	TEST_CASE("Invalidation disables copied and bound callables") {
		int calls = 0;
		Target target(calls);
		Callable original = callable_cpp(&target, &Target::add);
		Callable copy(original);
		Callable bound = original.bind(3);
		original = Callable();

		bound.call();
		CHECK(calls == 3);
		target.invalidate();
		CHECK_FALSE(copy.is_valid());
		CHECK_FALSE(bound.is_valid());
		CHECK(_call_error(bound) == GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL);
		CHECK(calls == 3);
	}

	TEST_CASE("Callables become invalid when the target is destroyed") {
		int calls = 0;
		Callable callable;
		{
			Target target(calls);
			callable = callable_cpp(&target, &Target::tick);
			callable.call();
		}
		CHECK_FALSE(callable.is_valid());
		CHECK(_call_error(callable) == GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL);
		CHECK(calls == 1);
	}

	TEST_CASE("Null targets produce invalid callables") {
		CHECK_FALSE(callable_cpp(static_cast<Target *>(nullptr), &Target::tick).is_valid());
	}

	TEST_CASE("The same target and method compare equal with stable hashes") {
		int calls = 0;
		Target target(calls);
		Target other(calls);
		Callable first = callable_cpp(&target, &Target::tick);
		Callable second = callable_cpp(&target, &Target::tick);
		CHECK(first == second);
		CHECK(first.hash() == second.hash());
		CHECK(first != callable_cpp(&other, &Target::tick));
		CHECK(first != callable_cpp(&target, &Target::tick_twice));
		const int64_t hash = first.hash();
		target.invalidate();
		CHECK(first == second);
		CHECK(first.hash() == hash);
	}

	TEST_CASE("Callables for different targets at the same address compare unequal") {
		// WHY IT WORKS: Each target gets a separately allocated Lifetime::State.
		// The old callable keeps the first state allocated after its target is destroyed,
		// so the new target gets a different state address.
		int calls = 0;
		alignas(Target) unsigned char storage[sizeof(Target)];
		Target *first = memnew_placement(storage, Target(calls));
		Callable old_callable = callable_cpp(first, &Target::tick);
		first->~Target();
		Target *second = memnew_placement(storage, Target(calls));
		Callable new_callable = callable_cpp(second, &Target::tick);
		CHECK_FALSE(old_callable.is_valid());
		CHECK(old_callable != new_callable);
		new_callable.call();
		CHECK(calls == 1);
		second->~Target();
	}

	TEST_CASE("Signals can disconnect using a newly constructed matching callable") {
		int calls = 0;
		Target target(calls);
		Node *node = memnew(Node);
		CHECK(node->connect("renamed", callable_cpp(&target, &Target::tick)) == OK);
		node->emit_signal("renamed");
		CHECK(calls == 1);
		node->disconnect("renamed", callable_cpp(&target, &Target::tick));
		node->emit_signal("renamed");
		CHECK(calls == 1);
		memdelete(node);
	}

	TEST_CASE("Signals do not invoke destroyed targets") {
		int calls = 0;
		Node *node = memnew(Node);
		{
			Target target(calls);
			CHECK(node->connect("renamed", callable_cpp(&target, &Target::tick)) == OK);
			node->emit_signal("renamed");
			CHECK(calls == 1);
		}
		node->emit_signal("renamed");
		CHECK(calls == 1);
		memdelete(node);
	}
}

#endif // TESTS_ENABLED
