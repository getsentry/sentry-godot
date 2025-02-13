extends GdUnitTestSuite
## Test SentrySDK methods.


signal callback_processed


## SentrySDK.capture_message() should return a non-empty event ID, which must match the ID returned by the get_last_event_id() call.
func test_capture_message() -> void:
	var event_id := SentrySDK.capture_message("capture_message_test", SentrySDK.LEVEL_DEBUG)
	assert_str(event_id).is_not_empty()
	assert_str(SentrySDK.get_last_event_id()).is_not_empty()
	assert_str(event_id).is_equal(SentrySDK.get_last_event_id())


## SentrySDK.set_tag() should assign a tag to the event object.
func test_set_tag() -> void:
	SentrySDK._set_before_send(
		func(ev: SentryEvent):
			assert_str(ev.get_tag("custom-tag")).is_equal("custom-tag-value")
			callback_processed.emit()
			return null)

	SentrySDK.set_tag("custom-tag", "custom-tag-value")

	var monitor := monitor_signals(self, false)
	SentrySDK.capture_message("test-tags")
	await assert_signal(monitor).is_emitted("callback_processed")

	SentrySDK._unset_before_send()


## SentrySDK.remove_tag() should remove a tag from the event object.
func test_remove_tag() -> void:
	SentrySDK._set_before_send(
		func(ev: SentryEvent):
			assert_str(ev.get_tag("custom-tag")).is_empty()
			callback_processed.emit()
			return null)

	SentrySDK.set_tag("custom-tag", "custom-tag-value")
	SentrySDK.remove_tag("custom-tag")

	var monitor := monitor_signals(self, false)
	SentrySDK.capture_message("test-tags")
	await assert_signal(monitor).is_emitted("callback_processed")

	SentrySDK._unset_before_send()
