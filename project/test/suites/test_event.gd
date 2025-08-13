extends GutTest
## Basic tests for the SentryEvent class.


## SentryEvent.id should not be empty on event creation.
func test_event_id() -> void:
	var event := SentrySDK.create_event()
	assert_ne(event.id, "")


## SentryEvent.message should be set to the specified value, and should be empty on event creation.
func test_event_message() -> void:
	var event := SentrySDK.create_event()
	assert_eq(event.message, "")
	event.message = "Hello, World!"
	assert_eq(event.message, "Hello, World!")


## SentryEvent.level should be set to the specified value.
func test_event_level() -> void:
	var event := SentrySDK.create_event()
	for l in [SentrySDK.LEVEL_DEBUG, SentrySDK.LEVEL_INFO, SentrySDK.LEVEL_WARNING, SentrySDK.LEVEL_ERROR, SentrySDK.LEVEL_FATAL]:
		event.level = l
		assert_eq(event.level, l)


## SentryEvent.timestamp should not be empty on event creation, and setter should update it.
func test_event_timestamp() -> void:
	var event := SentrySDK.create_event()

	# Test that timestamp is not null on creation
	assert_not_null(event.timestamp)

	# Test assigning a custom timestamp from microseconds
	var custom_timestamp := SentryTimestamp.from_microseconds_since_unix_epoch(1612325106123456)
	event.timestamp = custom_timestamp
	assert_true(event.timestamp.equals(custom_timestamp))
	assert_eq(event.timestamp.microseconds_since_unix_epoch, 1612325106123456)

	# Test assigning timestamp from current time
	var current_timestamp := SentryTimestamp.from_unix_time(Time.get_unix_time_from_system())
	event.timestamp = current_timestamp
	assert_true(event.timestamp.equals(current_timestamp))
	assert_eq(event.timestamp.microseconds_since_unix_epoch, current_timestamp.microseconds_since_unix_epoch)

	# Test assigning parsed RFC3339 timestamp
	var parsed_timestamp := SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06.123456789Z")
	event.timestamp = parsed_timestamp
	assert_true(event.timestamp.equals(parsed_timestamp))
	assert_eq(event.timestamp.microseconds_since_unix_epoch, 1612325106123456)


## SentryEvent.platform should not be empty.
func test_event_platform() -> void:
	var event := SentrySDK.create_event()
	assert_ne(event.platform, "")


## SentryEvent.logger should be set to the specified value, and empty on event creation.
func test_event_logger() -> void:
	var event := SentrySDK.create_event()
	assert_eq(event.logger, "")
	event.logger = "custom-logger"
	assert_eq(event.logger, "custom-logger")


## SentryEvent.release should be set to the specified value, and empty on event creation.
func test_event_release() -> void:
	var event := SentrySDK.create_event()
	assert_eq(event.release, "")
	event.release = "custom-release"
	assert_eq(event.release, "custom-release")


## SentryEvent.dist should be set to the specified value, and empty on event creation.
func test_event_dist() -> void:
	var event := SentrySDK.create_event()
	assert_eq(event.dist, "")
	event.dist = "custom-dist"
	assert_eq(event.dist, "custom-dist")


## SentryEvent.environment should be set to the specified value, and empty on event creation.
func test_event_environment() -> void:
	var event := SentrySDK.create_event()
	assert_eq(event.environment, "")
	event.environment = "custom-environment"
	assert_eq(event.environment, "custom-environment")


## SentryEvent.set_tag() should set tag to the specified value, remove_tag() should unset it.
func test_event_tags() -> void:
	var event := SentrySDK.create_event()
	assert_eq(event.get_tag("test_tag"), "")
	event.set_tag("test_tag", "test_value")
	assert_eq(event.get_tag("test_tag"), "test_value")
	event.remove_tag("test_tag")
	assert_eq(event.get_tag("test_tag"), "")


## SentryEvent.is_crash() should return false on a custom-created event.
func test_event_is_crash() -> void:
	var event := SentrySDK.create_event()
	assert_false(event.is_crash())


## SentrySDK.capture_event() should return a non-empty event ID, which must match the ID returned by the get_last_event_id() call.
func test_capture_event() -> void:
	var event := SentrySDK.create_event()
	var event_id := SentrySDK.capture_event(event)
	assert_ne(event_id, "")
	assert_eq(event_id, event.id)
	assert_ne(SentrySDK.get_last_event_id(), "")
	assert_eq(event_id, SentrySDK.get_last_event_id())
