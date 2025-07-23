#include "timestamp.h"

#include "sentry/util/print.h"

#include <regex>

namespace sentry::util {

bool is_valid_timestamp(const godot::String &p_timestamp) {
	static const std::regex rfc3339_regex{
		R"(^(?:[0-9]{4}-[0-9]{2}-[0-9]{2})T(?:[0-9]{2}:[0-9]{2}:[0-9]{2})(?:\.[0-9]+)?(?:Z|[\+\-][0-9]{2}:[0-9]{2})$)"
	};
	return std::regex_match(p_timestamp.ascii().ptr(), rfc3339_regex);
}

int64_t rfc3339_timestamp_to_microseconds(const char *p_formatted_cstring) {
	if (p_formatted_cstring == NULL) {
		return 0;
	}

	const char *cur = p_formatted_cstring;

	size_t len = strlen(p_formatted_cstring);
	if (len < 20) {
		return 0;
	}

	int year, month, day, hour, minute, second = 0;
	int64_t nanos = 0;

	int num_consumed = 0;
	constexpr int num_inputs = 6;
	constexpr int num_chars_in_date = 19;
	if (sscanf(cur, "%d-%d-%dT%d:%d:%d%n", &year, &month, &day, &hour, &minute, &second, &num_consumed) < num_inputs || num_consumed != num_chars_in_date) {
		return 0;
	}

	cur += num_consumed;

	if (cur[0] == '.') {
		int fractional = 0;
		if (sscanf(cur, ".%d%n", &fractional, &num_consumed) < 1 || num_consumed > 10) {
			sentry::util::print_error("Timestamp parsing needs 1-9 fractional digits.");
			return 0;
		}
		cur += num_consumed;

		// normalize to nanoseconds
		int missing_digits = 10 - num_consumed;
		while (missing_digits--) {
			fractional *= 10;
		}
		nanos = fractional;
	}

	// Handle timezone offset
	int timezone_offset_seconds = 0;
	if (cur[0] == 'Z') {
		// UTC timezone, no offset
		timezone_offset_seconds = 0;
	} else if (cur[0] == '+' || cur[0] == '-') {
		// Parse timezone offset (+HH:MM or -HH:MM)
		int sign = (cur[0] == '+') ? 1 : -1;
		int offset_hours, offset_minutes;

		if (sscanf(cur, "%*c%d:%d%n", &offset_hours, &offset_minutes, &num_consumed) < 2 || num_consumed != 6) {
			sentry::util::print_error("Invalid timezone offset format. Expected +HH:MM or -HH:MM");
			return 0;
		}

		timezone_offset_seconds = sign * (offset_hours * 3600 + offset_minutes * 60);
	} else {
		sentry::util::print_error("Invalid timezone format. Expected 'Z', '+HH:MM', or '-HH:MM'");
		return 0;
	}

	struct tm tm;
	tm.tm_year = year - 1900; // tm epoch is 1900
	tm.tm_mon = month - 1; // months are 0-based
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = minute;
	tm.tm_sec = second;
#ifdef WINDOWS_ENABLED
	time_t time = _mkgmtime(&tm);
#else
	time_t time = timegm(&tm);
#endif

	if (time == -1) {
		return 0;
	}

	// Apply timezone offset to convert to UTC
	time -= timezone_offset_seconds;

	int64_t microseconds = (int64_t)time * 1000000 + nanos / 1000;
	return microseconds;
}

godot::String microseconds_to_rfc3339_timestamp(int64_t p_microseconds_since_unix_epoch) {
	int64_t seconds = p_microseconds_since_unix_epoch / 1000000;
	int64_t remaining_microseconds = p_microseconds_since_unix_epoch % 1000000;

	time_t secs = static_cast<time_t>(seconds);
	struct tm *tm;

#if defined(PLATFORM_WINDOWS)
	tm = gmtime(&secs);
#else
	struct tm tm_buf;
	tm = gmtime_r(&secs, &tm_buf);
#endif

	if (!tm || tm->tm_year > 9000) {
		sentry::util::print_error("Failed to format timestamp");
		return String();
	}

	// YYYY-MM-DDTHH:MM:SS.ssssssZ
	return vformat("%04d-%02d-%02dT%02d:%02d:%02d.%06dZ",
			1900 + tm->tm_year,
			1 + tm->tm_mon, // month is 0-based
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec,
			remaining_microseconds);
}

} // namespace sentry::util
