class_name TestEventIntegrity
extends GdUnitTestSuite

signal callback_processed

var created_id: String


func before_test() -> void:
	SentrySDK.set_before_send(_before_send)


func after_test() -> void:
	SentrySDK.unset_before_send()


@warning_ignore("unused_parameter")
func test_event_integrity(timeout := 10000) -> void:
	var event := SentrySDK.create_event()
	event.message = "integrity-check"
	event.level = SentrySDK.LEVEL_DEBUG
	event.logger = "custom-logger"
	event.release = "custom-release"
	event.dist = "custom-dist"
	event.environment = "custom-environment"
	created_id = event.id

	var captured_id := SentrySDK.capture_event(event)
	assert_signal(self).wait_until(2000).is_emitted("callback_processed")

	assert_str(captured_id).is_not_empty()
	assert_str(captured_id).is_not_equal(created_id) # event was discarded
	assert_str(SentrySDK.get_last_event_id()).is_not_empty()
	assert_str(captured_id).is_equal(SentrySDK.get_last_event_id())
	assert_str(created_id).is_not_equal(SentrySDK.get_last_event_id())


func _before_send(event: SentryEvent) -> SentryEvent:
	assert_str(event.message).is_equal("integrity-check")
	assert_int(event.level).is_equal(SentrySDK.LEVEL_DEBUG)
	assert_str(event.logger).is_equal("custom-logger")
	assert_str(event.release).is_equal("custom-release")
	assert_str(event.dist).is_equal("custom-dist")
	assert_str(event.environment).is_equal("custom-environment")
	assert_str(event.id).is_equal(created_id)
	callback_processed.emit()
	return null # discard event
