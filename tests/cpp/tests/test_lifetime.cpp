#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/util/lifetime.h"

#include <utility>

using sentry::util::WeakRef;

namespace {

class Observable {
	LIFETIME_OBSERVABLE();

public:
	void invalidate() { _invalidate_lifetime(); }
};

} //namespace

TEST_SUITE("WeakRef") {
	TEST_CASE("Empty and null weak references have no target") {
		WeakRef<Observable> empty;
		WeakRef<Observable> null(nullptr);
		CHECK_FALSE(empty.is_alive());
		CHECK(empty.get() == nullptr);
		CHECK_FALSE(null.is_alive());
		CHECK(null.get() == nullptr);
	}

	TEST_CASE("Weak references become invalid when the target is destroyed") {
		WeakRef<Observable> weak_ref;
		WeakRef<const Observable> const_weak_ref;
		{
			Observable target;
			weak_ref = WeakRef<Observable>(&target);
			const_weak_ref = WeakRef<const Observable>(&target);
			CHECK(weak_ref.is_alive());
			CHECK(weak_ref.get() == &target);
			CHECK(const_weak_ref.get() == &target);
		}
		CHECK_FALSE(weak_ref.is_alive());
		CHECK(weak_ref.get() == nullptr);
		CHECK(const_weak_ref.get() == nullptr);
	}

	TEST_CASE("Explicit invalidation affects existing and subsequent weak references") {
		Observable target;
		WeakRef<Observable> first(&target);
		WeakRef<Observable> second(first);

		target.invalidate();
		target.invalidate();
		CHECK_FALSE(first.is_alive());
		CHECK(first.get() == nullptr);
		CHECK(second.get() == nullptr);
		CHECK(WeakRef<Observable>(&target).get() == nullptr);
	}

	TEST_CASE("Copies retain shared lifetime state") {
		Observable target;
		WeakRef<Observable> original(&target);
		WeakRef<Observable> copy(original);
		original = WeakRef<Observable>();
		CHECK(copy.get() == &target);
		target.invalidate();
		CHECK(copy.get() == nullptr);
	}

	TEST_CASE("Assignment replaces the observed lifetime") {
		Observable first_target;
		WeakRef<Observable> weak_ref(&first_target);
		{
			Observable second_target;
			WeakRef<Observable> second(&second_target);
			weak_ref = second;
			first_target.invalidate();
			CHECK(weak_ref.get() == &second_target);
		}
		CHECK(weak_ref.get() == nullptr);
	}

	TEST_CASE("Moves transfer observation and leave the source empty") {
		WeakRef<Observable> moved;
		{
			Observable target;
			WeakRef<Observable> source(&target);
			WeakRef<Observable> temporary(std::move(source));
			CHECK(source.get() == nullptr);
			CHECK(temporary.get() == &target);
			moved = std::move(temporary);
			CHECK(temporary.get() == nullptr);
			CHECK(moved.get() == &target);
		}
		CHECK(moved.get() == nullptr);
	}
}

#endif // TESTS_ENABLED
