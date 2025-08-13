extends GutTest
## Tests for SentryTimestamp class.


## Test creating SentryTimestamp from microseconds
func test_from_microseconds() -> void:
	# Test with whole seconds (no fractional part)
	var timestamp := SentryTimestamp.from_microseconds_since_unix_epoch(1612325106000000)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106000000)

	# Test with microseconds precision
	timestamp = SentryTimestamp.from_microseconds_since_unix_epoch(1612325106123456)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106123456)

	# Test with zero epoch
	timestamp = SentryTimestamp.from_microseconds_since_unix_epoch(0)
	assert_eq(timestamp.microseconds_since_unix_epoch, 0)


## Test creating SentryTimestamp from unix time (double)
func test_from_unix_time() -> void:
	# Test with whole seconds
	var timestamp := SentryTimestamp.from_unix_time(1612325106.0)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106000000)

	# Test with fractional seconds (microsecond precision)
	timestamp = SentryTimestamp.from_unix_time(1612325106.123456)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106123456)

	# Test with current time
	var current_time := Time.get_unix_time_from_system()
	timestamp = SentryTimestamp.from_unix_time(current_time)
	assert_not_null(timestamp)
	assert_gt(timestamp.microseconds_since_unix_epoch, 0)


## Test parsing RFC3339 formatted timestamp strings
func test_parse_rfc3339() -> void:
	# Test basic ISO 8601 format with Z timezone
	var timestamp := SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06Z")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106000000)

	# Test with milliseconds
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06.123Z")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106123000)

	# Test with microseconds
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06.123456Z")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106123456)

	# Test with nanoseconds (should truncate to microseconds)
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06.123456789Z")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106123456)

	# Test with positive timezone offset
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T06:05:06+02:00")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106000000)

	# Test with negative timezone offset
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T02:05:06-02:00")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106000000)


## Test timestamp equality comparison
func test_equals() -> void:
	# Use realistic timestamp: 2021-02-03T04:05:06.123456Z
	var timestamp1 := SentryTimestamp.from_microseconds_since_unix_epoch(1612325106123456)
	var timestamp2 := SentryTimestamp.from_microseconds_since_unix_epoch(1612325106123456)
	var timestamp3 := SentryTimestamp.from_microseconds_since_unix_epoch(1612325106123457) # +1 microsecond
	var timestamp4 := SentryTimestamp.from_microseconds_since_unix_epoch(1612325105123456) # -1 second

	# Test equal timestamps
	assert_true(timestamp1.equals(timestamp2))
	assert_true(timestamp2.equals(timestamp1))

	# Test different by single microsecond
	assert_false(timestamp1.equals(timestamp3))
	assert_false(timestamp3.equals(timestamp1))

	# Test different by seconds
	assert_false(timestamp1.equals(timestamp4))
	assert_false(timestamp4.equals(timestamp1))

	# Test self-equality
	assert_true(timestamp1.equals(timestamp1))

	# Test zero timestamp
	var zero1 := SentryTimestamp.from_microseconds_since_unix_epoch(0)
	var zero2 := SentryTimestamp.from_microseconds_since_unix_epoch(0)
	assert_true(zero1.equals(zero2))


## Test RFC3339 output and string representation
func test_rfc3339_output() -> void:
	var timestamp := SentryTimestamp.from_microseconds_since_unix_epoch(1612325106123456)
	var rfc3339 := timestamp.to_rfc3339()

	# Should follow RFC3339/ISO 8601 format with microseconds
	assert_eq(rfc3339, "2021-02-03T04:05:06.123456Z")

	# Test string representation (should be same as to_rfc3339)
	var string_repr := str(timestamp)
	assert_eq(string_repr, rfc3339)

	# Test with zero microseconds
	timestamp = SentryTimestamp.from_microseconds_since_unix_epoch(1612325106000000)
	rfc3339 = timestamp.to_rfc3339()
	assert_eq(rfc3339, "2021-02-03T04:05:06.000000Z")

	# Test epoch time formatting
	timestamp = SentryTimestamp.from_microseconds_since_unix_epoch(0)
	rfc3339 = timestamp.to_rfc3339()
	assert_eq(rfc3339, "1970-01-01T00:00:00.000000Z")


## Test invalid RFC3339 parsing (should handle gracefully)
func test_invalid_rfc3339_parsing() -> void:
	# Test with invalid format - should return null
	var timestamp := SentryTimestamp.parse_rfc3339("invalid-date-format")
	assert_null(timestamp)

	# Test with empty string
	timestamp = SentryTimestamp.parse_rfc3339("")
	assert_null(timestamp)

	# Test with partially valid format
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03")
	assert_null(timestamp)

	# Test with too short string
	timestamp = SentryTimestamp.parse_rfc3339("2021")
	assert_null(timestamp)

	# Test with invalid timezone format
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06+XX:XX")
	assert_null(timestamp)

	# Test with missing timezone
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06")
	assert_null(timestamp)


## Test advanced RFC3339 parsing features
func test_advanced_rfc3339_parsing() -> void:
	# Test various timezone offsets
	var timestamp := SentryTimestamp.parse_rfc3339("2021-02-03T09:05:06+05:00")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106000000)

	# Test negative timezone with minutes
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-02T23:35:06-04:30")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106000000)

	# Test UTC+0 and UTC-0
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06+00:00")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106000000)

	# Test fractional seconds precision
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06.1Z")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106100000)

	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06.12Z")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106120000)

	# Test nanosecond precision (should truncate to microseconds)
	timestamp = SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06.987654321Z")
	assert_not_null(timestamp)
	assert_eq(timestamp.microseconds_since_unix_epoch, 1612325106987654)
