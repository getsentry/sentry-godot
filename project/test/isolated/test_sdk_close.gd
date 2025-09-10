extends GdUnitTestSuite
## Test SDK should stop capture if closed via code.


signal callback_processed


func before_test() -> void:
	SentrySDK._set_before_send(_before_send)


func _before_send(_ev: SentryEvent) -> SentryEvent:
	callback_processed.emit()
	return null


func test_capture_when_sdk_closed() -> void:
	var monitor := monitor_signals(self, false)

	# SDK should auto initialize when enabled.
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message captured when SDK is initialiazed")
	await assert_signal(monitor).is_emitted("callback_processed")

	SentrySDK.close()
	assert_bool(SentrySDK.is_enabled()).is_false()

	SentrySDK.capture_message("message not captured when SDK is closed")
	await assert_signal(monitor).is_not_emitted("callback_processed")
