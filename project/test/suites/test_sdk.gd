extends GutTest
## Test SentrySDK methods.


signal callback_processed


## SentrySDK.capture_message() should return a non-empty event ID, which must match the ID returned by the get_last_event_id() call.
func test_capture_message() -> void:
	var event_id := SentrySDK.capture_message("capture_message_test", SentrySDK.LEVEL_DEBUG)
	assert_ne(event_id, "")
	assert_ne(SentrySDK.get_last_event_id(), "")
	assert_eq(event_id, SentrySDK.get_last_event_id())


## SentrySDK.set_tag() should assign a tag to the event object.
func test_set_tag() -> void:
	SentrySDK._set_before_send(
		func(ev: SentryEvent):
			assert_eq(ev.get_tag("custom-tag"), "custom-tag-value")
			callback_processed.emit()
			return null)

	SentrySDK.set_tag("custom-tag", "custom-tag-value")

	watch_signals(self)
	SentrySDK.capture_message("test-tags")
	await wait_for_signal(callback_processed, 2.0)
	assert_signal_emitted(callback_processed)

	SentrySDK._unset_before_send()


## SentrySDK.remove_tag() should remove a tag from the event object.
func test_remove_tag() -> void:
	SentrySDK._set_before_send(
		func(ev: SentryEvent):
			assert_eq(ev.get_tag("custom-tag"), "")
			callback_processed.emit()
			return null)

	SentrySDK.set_tag("custom-tag", "custom-tag-value")
	SentrySDK.remove_tag("custom-tag")

	watch_signals(self)
	SentrySDK.capture_message("test-tags")
	await wait_for_signal(callback_processed, 2.0)
	assert_signal_emitted(callback_processed)

	SentrySDK._unset_before_send()


## SentrySDK Variant conversion should not cause stack overflow.
func test_variant_conversion_against_stack_overflow() -> void:
	var dict: Dictionary = { "some_key": "some_value"}
	var arr: Array = [dict, "another_value"]
	dict["array"] = arr
	SentrySDK.set_context("broken_context", dict)
	assert_true(true, "didn't crash") # avoid warning in Gut
