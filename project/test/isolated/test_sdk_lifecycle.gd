extends GdUnitTestSuite
## Test lifecycle methods.


signal callback_processed


func _before_send(ev: SentryEvent) -> SentryEvent:
	if ev.is_crash():
		# Likely processing previous crash.
		return ev
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

	# NOTE: On Web, Sentry.close() is async - need to wait for it to complete.
	#       isEnabled only updates after flushing is finished.
	await get_tree().create_timer(1.0).timeout
	assert_bool(SentrySDK.is_enabled()).is_false()

	SentrySDK.capture_message("message not captured when SDK is closed")
	await assert_signal(self).is_not_emitted("callback_processed")


## Test that re-init creates fresh options (old before_send should not leak).
func test_reinit_clears_options() -> void:
	monitor_signals(self, false)

	# First init with before_send callback.
	SentrySDK.init(func (options: SentryOptions) -> void:
		options.before_send = _before_send
	)
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message triggers before_send")
	await assert_signal(self).is_emitted("callback_processed")

	SentrySDK.close()

	# Re-init WITHOUT callback -- old before_send should be gone.
	SentrySDK.init()
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message should not trigger old before_send")
	await assert_signal(self).is_not_emitted("callback_processed")

	SentrySDK.close()
