extends SentryTestSuite
## Basic tests for the SentryEvent class.


## Test string properties accessors and UTF-8 encoding preservation.
@warning_ignore("unused_parameter")
func test_string_properties_and_utf8(property: String, test_parameters := [
		["message"],
		["logger"],
		["release"],
		["dist"],
		["environment"],
]) -> void:
	var event := SentrySDK.create_event()
	event.set(property, "Hello, World!")
	assert_str(event.get(property)).is_equal("Hello, World!")
	event.set(property, "Hello 世界! 👋")
	assert_str(event.get(property)).is_equal("Hello 世界! 👋")


## SentryEvent.id should not be empty on event creation.
func test_event_id_not_empty() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.id).is_not_empty()


## SentryEvent.level should be set to the specified value.
func test_event_level() -> void:
	var event := SentrySDK.create_event()
	for l in [SentrySDK.LEVEL_DEBUG, SentrySDK.LEVEL_INFO, SentrySDK.LEVEL_WARNING, SentrySDK.LEVEL_ERROR, SentrySDK.LEVEL_FATAL]:
		event.level = l
		assert_int(event.level).is_equal(l)


## SentryEvent.timestamp should not be empty on event creation, and setter should update it.
func test_event_timestamp() -> void:
	var event := SentrySDK.create_event()

	# Test that timestamp is not null on creation
	assert_object(event.timestamp).is_not_null()

	# Test assigning a custom timestamp from microseconds
	var custom_timestamp := SentryTimestamp.from_microseconds_since_unix_epoch(1612325106123456)
	event.timestamp = custom_timestamp
	# NOTE: Millisecond precision on Android
	assert_int(event.timestamp.microseconds_since_unix_epoch).is_between(1612325106123000, 1612325106123456)

	# Test assigning timestamp from current time
	var current_timestamp := SentryTimestamp.from_unix_time(Time.get_unix_time_from_system())
	event.timestamp = current_timestamp
	assert_int(event.timestamp.microseconds_since_unix_epoch).is_between(
		current_timestamp.microseconds_since_unix_epoch - (current_timestamp.microseconds_since_unix_epoch % 1000), # remove microseconds data
		current_timestamp.microseconds_since_unix_epoch
	)

	# Test assigning parsed RFC3339 timestamp
	var parsed_timestamp := SentryTimestamp.parse_rfc3339("2021-02-03T04:05:06.123456789Z")
	event.timestamp = parsed_timestamp
	assert_int(event.timestamp.microseconds_since_unix_epoch).is_between(1612325106123000, 1612325106123456)


## SentryEvent.set_tag() should set tag to the specified value, remove_tag() should unset it.
func test_event_tags() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.get_tag("test_tag")).is_empty()
	event.set_tag("test_tag", "test_value")
	assert_str(event.get_tag("test_tag")).is_equal("test_value")
	event.remove_tag("test_tag")
	assert_str(event.get_tag("test_tag")).is_empty()
	event.set_tag("test_utf8", "Hello 世界! 👋")
	assert_str(event.get_tag("test_utf8")).is_equal("Hello 世界! 👋")


## SentryEvent.is_crash() should return false on a custom-created event.
func test_event_is_crash() -> void:
	var event := SentrySDK.create_event()
	assert_bool(event.is_crash()).is_false()


## SentrySDK.capture_event() should return a non-empty event ID, which must match the ID returned by the get_last_event_id() call.
func test_capture_event() -> void:
	# Ensure events are not discarded.
	SentrySDK._set_before_send(func(ev): return ev)

	var event := SentrySDK.create_event()
	var event_id := SentrySDK.capture_event(event)

	assert_str(event_id).is_not_empty()
	assert_str(event_id).is_equal(event.id)
	assert_str(SentrySDK.get_last_event_id()).is_not_empty()
	assert_str(event_id).is_equal(SentrySDK.get_last_event_id())
