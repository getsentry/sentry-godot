extends SentryTestSuite
## Verify that event properties are preserved through the SDK flow.


signal callback_processed

var created_id: String


@warning_ignore("unused_parameter")
func test_event_integrity(timeout := 10000) -> void:
	_capture_event()
	await assert_signal(self).is_emitted("callback_processed")


@warning_ignore("unused_parameter")
func test_threaded_event_capture(timeout := 10000) -> void:
	var thread := Thread.new()
	thread.start(_capture_event)
	await assert_signal(self).is_emitted("callback_processed")
	thread.wait_to_finish()


func _capture_event() ->  void:
	SentrySDK._set_before_send(_before_send)

	var event := SentrySDK.create_event()
	event.message = "integrity-check"
	event.level = SentrySDK.LEVEL_DEBUG
	event.logger = "custom-logger"
	event.release = "custom-release"
	event.dist = "custom-dist"
	event.environment = "custom-environment"
	event.set_tag("custom-tag", "custom-tag-value")
	created_id = event.id

	SentrySDK.capture_event(event)


func _before_send(event: SentryEvent) -> SentryEvent:
	assert_str(event.message).is_equal("integrity-check")
	assert_int(event.level).is_equal(SentrySDK.LEVEL_DEBUG)
	assert_str(event.logger).is_equal("custom-logger")
	assert_str(event.release).is_equal("custom-release")
	assert_str(event.dist).is_equal("custom-dist")
	assert_str(event.environment).is_equal("custom-environment")
	assert_str(event.get_tag("custom-tag")).is_equal("custom-tag-value")
	assert_str(event.id).is_equal(created_id)
	assert_bool(event.is_crash()).is_false()
	callback_processed.emit.call_deferred()
	return null # discard event
