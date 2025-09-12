extends GdUnitTestSuite
## Events should not be logged for errors when the logger is disabled.


signal callback_processed

var _num_events: int = 0


func before() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.logger_enabled = false

		# Make sure other limits are not interfering.
		options.logger_limits.events_per_frame = 88
		options.logger_limits.throttle_events = 88
		options.logger_limits.repeated_error_window_ms = 0
		options.logger_limits.throttle_window_ms = 0

		options.before_send = _before_send
	)


func _before_send(_ev: SentryEvent) -> SentryEvent:
	_num_events += 1
	callback_processed.emit()
	return null


func test_event_and_breadcrumb_masks() -> void:
	var monitor := monitor_signals(self, false)
	push_error("dummy-error")
	push_warning("dummy-warning")
	await assert_signal(monitor).is_not_emitted("callback_processed")

	assert_int(_num_events).is_equal(0)
