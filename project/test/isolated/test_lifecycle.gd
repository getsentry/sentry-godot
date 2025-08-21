extends GdUnitTestSuite
## Test lifecycle methods.


signal callback_processed


static func configure_options(options: SentryOptions) -> void:
	# Disable SDK auto-intialization.
	options.enabled = false


func before_test() -> void:
	SentrySDK._set_before_send(_before_send)


func _before_send(_ev: SentryEvent) -> SentryEvent:
	callback_processed.emit()
	return null


## Test manual initialization and shutdown of SDK.
func test_lifecycle() -> void:
	var monitor := monitor_signals(self, false)

	# SDK should be disabled when `options.enabled = false`
	assert_bool(SentrySDK.is_enabled()).is_false()

	SentrySDK.capture_message("message not captured before SDK is initialized")
	await assert_signal(monitor).is_not_emitted("callback_processed")

	SentrySDK.init()
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message captured when SDK is initialiazed")
	await assert_signal(monitor).is_emitted("callback_processed")

	SentrySDK.close()
	assert_bool(SentrySDK.is_enabled()).is_false()

	SentrySDK.capture_message("message not captured when SDK is closed")
	await assert_signal(monitor).is_not_emitted("callback_processed")
