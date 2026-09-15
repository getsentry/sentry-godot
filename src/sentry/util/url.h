#pragma once

#include <cstdint>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::util {

// The components of a parsed URL without credentials.
struct URLParts {
	String scheme;
	String host;
	// -1 when the port is omitted or empty.
	int32_t port = -1;
	String path;
	String query;
	String fragment;
	bool has_authority = false;

	// Rebuilds the URL without credentials, query, or fragment.
	String redacted() const;
};

// Parses a URL into components, discarding credentials.
// Recognizes authority after "scheme://" or a leading "//"; otherwise parses a path, query, and fragment.
// Returns ERR_INVALID_DATA for malformed authority components, leaving r_parts unchanged.
Error parse_url(const String &p_url, URLParts &r_parts);

} // namespace sentry::util
