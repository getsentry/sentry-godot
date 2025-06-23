#include "timestamp.h"

#include <regex>

bool sentry::util::is_valid_timestamp(const godot::String &p_timestamp) {
	static const std::regex rfc3339_regex{
		R"(^(?:[0-9]{4}-[0-9]{2}-[0-9]{2})T(?:[0-9]{2}:[0-9]{2}:[0-9]{2})(?:\.[0-9]+)?(?:Z|[\+\-][0-9]{2}:[0-9]{2})$)"
	};
	return std::regex_match(p_timestamp.ascii().ptr(), rfc3339_regex);
}
