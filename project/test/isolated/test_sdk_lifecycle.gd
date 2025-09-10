extends GdUnitTestSuite
## Test lifecycle methods.


signal callback_processed


func _before_send(_ev: SentryEvent) -> SentryEvent:
	callback_processed.emit()
	return null


## Test manual initialization and shutdown of SDK.
func test_sdk_lifecycle() -> void:
	monitor_signals(self, false)

	# SDK should be disabled at start.
	assert_bool(SentrySDK.is_enabled()).is_false()

	SentrySDK.capture_message("message not captured before SDK is initialized")
	await assert_signal(self).is_not_emitted("callback_processed")

	SentrySDK.init(func (options: SentryOptions) -> void:
		options.before_send = _before_send
	)
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message captured when SDK is initialiazed")
	await assert_signal(self).is_emitted("callback_processed")

	SentrySDK.close()
	assert_bool(SentrySDK.is_enabled()).is_false()

	SentrySDK.capture_message("message not captured when SDK is closed")
	await assert_signal(self).is_not_emitted("callback_processed")
