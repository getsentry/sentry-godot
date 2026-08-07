extends GdUnitTestSuite
## Test lifecycle methods.


signal callback_processed

var _captured_session_tag: String
var _captured_event_json: String


func _before_send(ev: SentryEvent) -> SentryEvent:
	if ev.is_crash():
		# Likely processing previous crash.
		return ev
	_captured_session_tag = ev.get_tag("session")
	_captured_event_json = ev.to_json()
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
		options.shutdown_timeout_ms = 2000
	)
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message captured when SDK is initialiazed")
	await assert_signal(self).is_emitted("callback_processed")

	SentrySDK.close()

	# NOTE: On Web, Sentry.close() is async - need to wait for it to complete.
	#       isEnabled only updates after flushing is finished.
	await get_tree().create_timer(2.0).timeout
	assert_bool(SentrySDK.is_enabled()).is_false()

	SentrySDK.capture_message("message not captured when SDK is closed")
	await assert_signal(self).is_not_emitted("callback_processed")


## Test that re-init creates fresh options (old before_send should not leak).
func test_reinit_clears_options() -> void:
	monitor_signals(self, false)

	# First init with before_send callback.
	SentrySDK.init(func (options: SentryOptions) -> void:
		options.before_send = _before_send
		options.shutdown_timeout_ms = 2000
	)
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message triggers before_send")
	await assert_signal(self).is_emitted("callback_processed")

	SentrySDK.close()

	# NOTE: On Web, Sentry.close() is async - need to wait for it to complete.
	#       isEnabled only updates after flushing is finished.
	await get_tree().create_timer(2.0).timeout

	# Re-init WITHOUT callback -- old before_send should be gone.
	SentrySDK.init()
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message should not trigger old before_send")
	await assert_signal(self).is_not_emitted("callback_processed")

	SentrySDK.close()

	# NOTE: On Web, Sentry.close() is async - let it finish before the next test initializes.
	await get_tree().create_timer(2.0).timeout


## Test that re-init clears globally set data (old tags and breadcrumbs should not leak).
func test_reinit_clears_global_data() -> void:
	monitor_signals(self, false)

	SentrySDK.init(func (options: SentryOptions) -> void:
		options.before_send = _before_send
		options.shutdown_timeout_ms = 2000
	)

	SentrySDK.set_tag("session", "first")
	SentrySDK.add_breadcrumb(SentryBreadcrumb.create("first session breadcrumb"))
	SentrySDK.capture_message("message from the first session")
	await assert_signal(self).is_emitted("callback_processed")
	assert_str(_captured_session_tag).is_equal("first")
	assert_str(_captured_event_json).contains("first session breadcrumb")

	SentrySDK.close()

	# NOTE: On Web, Sentry.close() is async - need to wait for it to complete.
	await get_tree().create_timer(2.0).timeout

	# Re-init without setting them again -- the data from the first session should be gone.
	SentrySDK.init(func (options: SentryOptions) -> void:
		options.before_send = _before_send
		options.shutdown_timeout_ms = 2000
	)

	SentrySDK.capture_message("message from the second session")
	await assert_signal(self).is_emitted("callback_processed")
	assert_str(_captured_session_tag).is_empty()
	assert_str(_captured_event_json).not_contains("first session breadcrumb")

	SentrySDK.close()
