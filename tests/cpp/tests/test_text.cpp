#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/util/text.h"

using sentry::util::string_from_span;

TEST_CASE("UTF-32 spans copy only their range into an owning String") {
	char32_t buffer[] = { U'x', U'雪', U'😀', U'y' };
	const godot::String result = string_from_span(godot::Span<char32_t>(buffer + 1, 2));
	buffer[1] = U'z';
	CHECK(result == godot::String(U"雪😀"));
	CHECK(result.length() == 2);
	CHECK(string_from_span(godot::Span<char32_t>()).is_empty());
	CHECK(string_from_span(godot::Span<char32_t>(buffer + 4, 0)).is_empty());
	buffer[2] = U'\0';
	CHECK(string_from_span(godot::Span<char32_t>(buffer, 4)) == "xz");
}

#endif // TESTS_ENABLED
