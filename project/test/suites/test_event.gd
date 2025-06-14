extends GdUnitTestSuite
## Basic tests for the SentryEvent class.


## SentryEvent.id should not be empty on event creation.
func test_event_id() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.id).is_not_empty()


## SentryEvent.message should be set to the specified value, and should be empty on event creation.
func test_event_message() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.message).is_empty()
	event.message = "Hello, World!"
	assert_str(event.message).is_equal("Hello, World!")


## SentryEvent.level should be set to the specified value.
func test_event_level() -> void:
	var event := SentrySDK.create_event()
	for l in [SentrySDK.LEVEL_DEBUG, SentrySDK.LEVEL_INFO, SentrySDK.LEVEL_WARNING, SentrySDK.LEVEL_ERROR, SentrySDK.LEVEL_FATAL]:
		event.level = l
		assert_int(event.level).is_equal(l)


## SentryEvent.timestamp should not be empty on event creation, and setter should update it.
func test_event_timestamp() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.timestamp).is_not_empty()

	# Example of how to format a timestamp:
	var unix_time: float = Time.get_unix_time_from_system()
	var dt: Dictionary = Time.get_datetime_dict_from_unix_time(unix_time)
	var date := "%04d-%02d-%02d" % [dt.year, dt.month, dt.day]
	var time := "%02d:%02d:%02d" % [dt.hour, dt.minute, dt.second]
	var fractional: float = unix_time - int(unix_time)
	var micros := int(round(fractional * 1000000))
	var timestamp := "%sT%s.%06dZ" % [date, time, micros]

	event.timestamp = timestamp
	assert_str(event.timestamp).is_equal(timestamp)


## SentryEvent.platform should not be empty.
func test_event_platform() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.platform).is_not_empty()


## SentryEvent.logger should be set to the specified value, and empty on event creation.
func test_event_logger() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.logger).is_empty()
	event.logger = "custom-logger"
	assert_str(event.logger).is_equal("custom-logger")


## SentryEvent.release should be set to the specified value, and empty on event creation.
func test_event_release() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.release).is_empty()
	event.release = "custom-release"
	assert_str(event.release).is_equal("custom-release")


## SentryEvent.dist should be set to the specified value, and empty on event creation.
func test_event_dist() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.dist).is_empty()
	event.dist = "custom-dist"
	assert_str(event.dist).is_equal("custom-dist")


## SentryEvent.environment should be set to the specified value, and empty on event creation.
func test_event_environment() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.environment).is_empty()
	event.environment = "custom-environment"
	assert_str(event.environment).is_equal("custom-environment")


## SentryEvent.set_tag() should set tag to the specified value, remove_tag() should unset it.
func test_event_tags() -> void:
	var event := SentrySDK.create_event()
	assert_str(event.get_tag("test_tag")).is_empty()
	event.set_tag("test_tag", "test_value")
	assert_str(event.get_tag("test_tag")).is_equal("test_value")
	event.remove_tag("test_tag")
	assert_str(event.get_tag("test_tag")).is_empty()


## SentryEvent.is_crash() should return false on a custom-created event.
func test_event_is_crash() -> void:
	var event := SentrySDK.create_event()
	assert_bool(event.is_crash()).is_false()


## SentrySDK.capture_event() should return a non-empty event ID, which must match the ID returned by the get_last_event_id() call.
func test_capture_event() -> void:
	var event := SentrySDK.create_event()
	var event_id := SentrySDK.capture_event(event)
	assert_str(event_id).is_not_empty()
	assert_str(event_id).is_equal(event.id)
	assert_str(SentrySDK.get_last_event_id()).is_not_empty()
	assert_str(event_id).is_equal(SentrySDK.get_last_event_id())
