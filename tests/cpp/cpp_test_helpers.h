#pragma once

#ifdef TESTS_ENABLED

#include <doctest.h>

#include <godot_cpp/core/defs.hpp>
#include <ostream>
#include <string_view>
#include <vector>

namespace sentry::tests {

// Wraps string_view so doctest renders \r \n \t as visible.
struct Visible {
	std::string_view s;
};

inline std::ostream &operator<<(std::ostream &p_os, const Visible &p_v) {
	for (char c : p_v.s) {
		switch (c) {
			case '\r':
				p_os << doctest::Color::Yellow << "\\r" << doctest::Color::None;
				break;
			case '\n':
				p_os << doctest::Color::Yellow << "\\n"
					 << doctest::Color::None << "\n";
				break;
			case '\t':
				p_os << doctest::Color::Yellow << "\\t" << doctest::Color::None;
				break;
			default:
				p_os.put(c);
				break;
		}
	}
	return p_os;
}

// Re-exposes a byte vector as a string_view without copying.
inline std::string_view as_view(const std::vector<std::uint8_t> &p_bytes) {
	return { reinterpret_cast<const char *>(p_bytes.data()), p_bytes.size() };
}

} // namespace sentry::tests

// Compares byte strings, and on mismatch, shows visible whitespace in doctest INFO.
// Defined as a macro so CHECK reports the call-site file:line, not this helper LoC.
#define CHECK_SNAPSHOT_EQ(p_actual, p_expected)                                             \
	do {                                                                                    \
		const std::string_view _e = (p_expected);                                           \
		const std::string_view _a = (p_actual);                                             \
		const bool match = (_a == _e);                                                      \
		INFO(doctest::Color::Cyan << "\n--- expected ---\n"                                 \
								  << doctest::Color::None << ::sentry::tests::Visible{ _e } \
								  << doctest::Color::Cyan << "--- actual ---\n"             \
								  << doctest::Color::None << ::sentry::tests::Visible{ _a } \
								  << doctest::Color::Cyan << "--- end ---\n");              \
		CHECK(match);                                                                       \
	} while (0)

#endif // TESTS_ENABLED
