class_name TestSDK
extends GdUnitTestSuite

## SentrySDK.capture_message() should return a non-empty event ID, which must match the ID returned by the get_last_event_id() call.
func test_capture_message() -> void:
	var event_id := SentrySDK.capture_message("capture_message_test", SentrySDK.LEVEL_DEBUG)
	assert_str(event_id).is_not_empty()
	assert_str(SentrySDK.get_last_event_id()).is_not_empty()
	assert_str(event_id).is_equal(SentrySDK.get_last_event_id())
