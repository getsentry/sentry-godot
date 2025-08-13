extends GutTest
## Verify that event properties are preserved through the SDK flow.


signal callback_processed

var created_id: String


func before_each() -> void:
	SentrySDK._set_before_send(_before_send)


func after_each() -> void:
	SentrySDK._unset_before_send()


func test_event_integrity() -> void:
	watch_signals(self)
	_capture_event()
	await wait_for_signal(callback_processed, 2.0)
	assert_signal_emitted(callback_processed)


func test_threaded_event_capture() -> void:
	watch_signals(self)

	var thread := Thread.new()
	thread.start(_capture_event)
	await wait_for_signal(callback_processed, 2.0)
	assert_signal_emitted(callback_processed)
	thread.wait_to_finish()


func _capture_event() ->  void:
	var event := SentrySDK.create_event()
	event.message = "integrity-check"
	event.level = SentrySDK.LEVEL_DEBUG
	event.logger = "custom-logger"
	event.release = "custom-release"
	event.dist = "custom-dist"
	event.environment = "custom-environment"
	event.set_tag("custom-tag", "custom-tag-value")
	created_id = event.id

	var captured_id := SentrySDK.capture_event(event)
	print("------ ID:", captured_id)
	assert_ne(captured_id, "")
	assert_ne(captured_id, created_id) # event was discarded
	# assert_eq(captured_id, SentrySDK.get_last_event_id()) // NOTE: inconsistent across SDKs
	assert_ne(created_id, SentrySDK.get_last_event_id())


func _before_send(event: SentryEvent) -> SentryEvent:
	print(event, " message: ", event.message)
	assert_eq(event.message, "integrity-check")
	assert_eq(event.level, SentrySDK.LEVEL_DEBUG)
	assert_eq(event.logger, "custom-logger")
	assert_eq(event.release, "custom-release")
	assert_eq(event.dist, "custom-dist")
	assert_eq(event.environment, "custom-environment")
	assert_eq(event.get_tag("custom-tag"), "custom-tag-value")
	assert_eq(event.id, created_id)
	assert_false(event.is_crash())
	callback_processed.emit.call_deferred()
	return null # discard event
