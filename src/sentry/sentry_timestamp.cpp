#include "sentry_timestamp.h"

#include "sentry/util/print.h"
#include "sentry/util/simple_bind.h"

#include <ctime>

namespace sentry {

Ref<SentryTimestamp> SentryTimestamp::parse_rfc3339_cstr(const char *p_cstring) {
	if (p_cstring == NULL) {
		return Ref<SentryTimestamp>();
	}

	const char *cur = p_cstring;

	size_t len = strlen(p_cstring);
	if (len < 20) {
		return Ref<SentryTimestamp>();
	}

	int year, month, day, hour, minute, second = 0;
	int64_t nanos = 0;

	int num_consumed = 0;
	constexpr int num_inputs = 6;
	constexpr int num_chars_in_date = 19;
	if (sscanf(cur, "%d-%d-%dT%d:%d:%d%n", &year, &month, &day, &hour, &minute, &second, &num_consumed) < num_inputs || num_consumed != num_chars_in_date) {
		return Ref<SentryTimestamp>();
	}

	cur += num_consumed;

	if (cur[0] == '.') {
		int fractional = 0;
		if (sscanf(cur, ".%d%n", &fractional, &num_consumed) < 1 || num_consumed > 10) {
			sentry::util::print_error("Timestamp parsing needs 1-9 fractional digits.");
			return Ref<SentryTimestamp>();
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
			return Ref<SentryTimestamp>();
		}

		timezone_offset_seconds = sign * (offset_hours * 3600 + offset_minutes * 60);
	} else {
		sentry::util::print_error("Invalid timezone format. Expected 'Z', '+HH:MM', or '-HH:MM'");
		return Ref<SentryTimestamp>();
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
		return Ref<SentryTimestamp>();
	}

	// Apply timezone offset to convert to UTC
	time -= timezone_offset_seconds;

	int64_t microseconds = (int64_t)time * 1000000 + nanos / 1000;

	Ref<SentryTimestamp> timestamp;
	timestamp.instantiate();
	timestamp->set_microseconds_since_epoch(microseconds);
	return timestamp;
}

Ref<SentryTimestamp> SentryTimestamp::from_microseconds(int64_t p_microseconds_since_epoch) {
	Ref<SentryTimestamp> ts;
	ts.instantiate();
	ts->set_microseconds_since_epoch(p_microseconds_since_epoch);
	return ts;
}

Ref<SentryTimestamp> SentryTimestamp::from_unix_time(double p_unix_time) {
	int64_t microseconds = (int64_t)round(p_unix_time * 1000000);

	Ref<SentryTimestamp> ts;
	ts.instantiate();
	ts->set_microseconds_since_epoch(microseconds);
	return ts;
}

String SentryTimestamp::to_rfc3339() const {
	int64_t seconds = microseconds_since_epoch / 1000000;
	int64_t remaining_microseconds = microseconds_since_epoch % 1000000;

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

void SentryTimestamp::_bind_methods() {
	ClassDB::bind_static_method("SentryTimestamp", D_METHOD("parse_rfc3339", "formatted_string"), &SentryTimestamp::parse_rfc3339);
	ClassDB::bind_static_method("SentryTimestamp", D_METHOD("from_microseconds", "microseconds_since_epoch"), &SentryTimestamp::from_microseconds);
	ClassDB::bind_static_method("SentryTimestamp", D_METHOD("from_unix_time", "unix_time"), &SentryTimestamp::from_unix_time);

	BIND_PROPERTY(SentryTimestamp, PropertyInfo(Variant::INT, "microseconds_since_epoch"), set_microseconds_since_epoch, get_microseconds_since_epoch);

	ClassDB::bind_method(D_METHOD("to_rfc3339"), &SentryTimestamp::to_rfc3339);
	ClassDB::bind_method(D_METHOD("equals", "other"), &SentryTimestamp::equals);
}

} // namespace sentry
