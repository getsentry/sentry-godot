extends SentryTestSuite
## Verify that event properties are preserved through the SDK flow.


signal callback_processed

var before_send: Callable # (event: SentryEvent) -> SentryEvent

var created_id: String


func _before_send(event: SentryEvent) -> SentryEvent:
	callback_processed.emit.call_deferred()
	return before_send.call(event)


@warning_ignore("unused_parameter")
func test_event_integrity(timeout := 10000) -> void:
	_capture_event("event integrity check from main thread")
	await assert_signal(self).is_emitted("callback_processed")


@warning_ignore("unused_parameter")
func test_threaded_event_capture(timeout := 10000) -> void:
	var thread := Thread.new()
	thread.start(_capture_event.bind("event integrity check from worker thread"))
	await assert_signal(self).is_emitted("callback_processed")
	thread.wait_to_finish()


func _capture_event(message: String) ->  void:
	SentrySDK._set_before_send(_before_send)

	var event := SentrySDK.create_event()
	event.message = message
	event.level = SentrySDK.LEVEL_DEBUG
	event.logger = "custom-logger"
	event.release = "custom-release"
	event.dist = "custom-dist"
	event.environment = "custom-environment"
	event.set_tag("custom-tag", "custom-tag-value")
	created_id = event.id

	before_send = func(ev: SentryEvent) -> SentryEvent:
		assert_str(ev.message).is_equal(message)
		assert_int(ev.level).is_equal(SentrySDK.LEVEL_DEBUG)
		assert_str(ev.logger).is_equal("custom-logger")
		assert_str(ev.release).is_equal("custom-release")
		assert_str(ev.dist).is_equal("custom-dist")
		assert_str(ev.environment).is_equal("custom-environment")
		assert_str(ev.get_tag("custom-tag")).is_equal("custom-tag-value")
		assert_str(ev.id).is_equal(created_id)
		assert_str(ev.platform).is_not_empty()
		assert_bool(ev.is_crash()).is_false()
		return null

	SentrySDK.capture_event(event)


func test_event_exception_interface() -> void:
	var error_message := "Testing 123"

	before_send = func(ev: SentryEvent) -> SentryEvent:
		assert_int(ev.get_exception_count()).is_greater_equal(1)
		assert_str(ev.get_exception_value(0)).is_equal("Testing 123")
		ev.set_exception_value(0, "New value")
		assert_str(ev.get_exception_value(0)).is_equal("New value")
		return null

	push_error(error_message)
	await assert_signal(self).is_emitted("callback_processed")
