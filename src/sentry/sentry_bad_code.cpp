#include "sentry_bad_code.h"

#include <godot_cpp/core/class_db.hpp>

#include <cstdlib>

namespace sentry {

void SentryBadCode::crash_with_null_dereference() {
	volatile int *ptr = nullptr;
	*ptr = 0;
}

void SentryBadCode::crash_with_stack_overflow() {
	volatile int arr[1024];
	crash_with_stack_overflow();
	arr[0] = 0;
}

void SentryBadCode::crash_with_abort() {
	std::abort();
}

void SentryBadCode::crash_with_division_by_zero() {
	volatile int a = 1;
	volatile int b = 0;
	volatile int c = a / b;
	(void)c;
}

void SentryBadCode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("crash_with_null_dereference"), &SentryBadCode::crash_with_null_dereference);
	ClassDB::bind_method(D_METHOD("crash_with_stack_overflow"), &SentryBadCode::crash_with_stack_overflow);
	ClassDB::bind_method(D_METHOD("crash_with_abort"), &SentryBadCode::crash_with_abort);
	ClassDB::bind_method(D_METHOD("crash_with_division_by_zero"), &SentryBadCode::crash_with_division_by_zero);
}

} // namespace sentry
