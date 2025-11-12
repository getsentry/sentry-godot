extends SentryTestSuite
## Test SentrySDK methods.


signal callback_processed


## SentrySDK.capture_message() should return a non-empty event ID, which must match the ID returned by the get_last_event_id() call.
func test_capture_message_id() -> void:
	# Ensure events are not discarded
	SentrySDK._set_before_send(func(ev): return ev)

	var event_id := SentrySDK.capture_message("capture_message_test", SentrySDK.LEVEL_DEBUG)

	assert_str(event_id).is_not_empty()
	assert_str(SentrySDK.get_last_event_id()).is_not_empty()
	assert_str(event_id).is_equal(SentrySDK.get_last_event_id())


## SentrySDK.set_tag() should assign a tag to the event object.
func test_set_tag() -> void:
	SentrySDK._set_before_send(
		func(ev: SentryEvent):
			assert_str(ev.get_tag("custom-tag")).is_equal("custom-tag-value")
			assert_str(ev.get_tag("utf8-test")).is_equal("Hello 世界! 👋")
			callback_processed.emit()
			return null)

	SentrySDK.set_tag("custom-tag", "custom-tag-value")
	SentrySDK.set_tag("utf8-test", "Hello 世界! 👋")

	var monitor := monitor_signals(self, false)
	SentrySDK.capture_message("test-tags")
	await assert_signal(monitor).is_emitted("callback_processed")


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


## SentrySDK Variant conversion should not cause stack overflow.
func test_variant_conversion_against_stack_overflow() -> void:
	var dict: Dictionary = { "some_key": "some_value"}
	var arr: Array = [dict, "another_value"]
	dict["array"] = arr
	SentrySDK.set_context("broken_context", dict)
	# Unset context
	SentrySDK.set_context("broken_context", {})
