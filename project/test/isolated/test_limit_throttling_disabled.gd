extends GdUnitTestSuite
## Test throttling should be disabled if the window is set to 0.


signal callback_processed

var _num_events: int = 0


func before() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		# Setting throttle window to 0 should disable throttling.
		options.logger_limits.throttle_window_ms = 0
		options.logger_limits.throttle_events = 1
		# Make sure other limits are not interfering.
		options.logger_limits.events_per_frame = 88
		options.before_send = _before_send
	)


func _before_send(ev: SentryEvent) -> SentryEvent:
	if ev.is_crash():
		# Likely processing previous crash.
		return ev
	_num_events += 1
	callback_processed.emit()
	return null


## All errors should be logged.
func test_throttling_window_set_to_zero() -> void:
	monitor_signals(self, false)
	push_error("dummy-error 1")
	push_error("dummy-error 2")
	push_error("dummy-error 3")
	push_error("dummy-error 4")
	await assert_signal(self).is_emitted("callback_processed")
	await get_tree().create_timer(0.1).timeout
	assert_int(_num_events).is_equal(4)
