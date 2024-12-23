class_name TestEvent
extends GdUnitTestSuite


## SentryEvent.id should be empty on event creation.
func test_event_id() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.id).is_empty()


## SentryEvent.message should be set to the specified value, and should be empty on event creation.
func test_event_message() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.message).is_empty()
	event.message = "Hello, World!"
	assert_str(event.message).is_equal("Hello, World!")


## SentryEvent.level should be set to the specified value.
func test_event_level() -> void:
	var event := SentrySDK.create_event()
	for l in [SentrySDK.LEVEL_INFO, SentrySDK.LEVEL_WARNING, SentrySDK.LEVEL_ERROR, SentrySDK.LEVEL_FATAL]:
		event.level = l
		assert_int(event.level).is_equal(l)


## SentryEvent.timestamp should not be empty on event creation, and setter should update it.
func test_event_timestamp() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.timestamp).is_not_empty()
	var ts = Time.get_datetime_string_from_system()
	event.timestamp = ts
	assert_str(event.timestamp).is_equal(ts)


## SentrySDK.capture_event() should return a non-empty event ID, which must match the ID returned by the get_last_event_id() call.
func test_capture_event() -> void:
	var event := SentrySDK.create_event()
	var event_id := SentrySDK.capture_event(event)
	assert_str(event_id).is_not_empty()
	assert_str(SentrySDK.get_last_event_id()).is_not_empty()
	assert_str(event_id).is_equal(SentrySDK.get_last_event_id())
