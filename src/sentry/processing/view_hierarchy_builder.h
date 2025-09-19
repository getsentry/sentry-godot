#pragma once

#include "sentry/util/utf8_buffer.h"

#include <godot_cpp/variant/string.hpp>

namespace sentry {

class ViewHierarchyBuilder {
private:
	// Initial estimated buffer size for JSON serialization (bytes).
	// This value is adjusted based on past data to minimize reallocations.
	size_t estimated_buffer_size = 262'144;

public:
	sentry::util::UTF8Buffer build_json();
};

} //namespace sentry
