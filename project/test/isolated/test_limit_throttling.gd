extends GdUnitTestSuite
## Test error logger throttling limits.


signal callback_processed

var _num_events: int = 0


func before() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		# Allow only two errors to be logged as events within 1 second time window.
		options.logger_limits.throttle_events = 2
		options.logger_limits.throttle_window_ms = 1000
		# Make sure other limits are not interfering.
		options.logger_limits.events_per_frame = 88
		options.logger_limits.repeated_error_window_ms = 0
		options.before_send = _before_send
	)


func _before_send(ev: SentryEvent) -> SentryEvent:
	if ev.is_crash():
		# Likely processing previous crash.
		return ev
	_num_events += 1
	callback_processed.emit()
	return null


## Only two errors should be logged within the assigned time window.
func test_throttling_limits() -> void:
	# Wait for special startup limits to expire.
	while Engine.get_process_frames() < 10:
		await get_tree().process_frame

	monitor_signals(self, false)

	push_error("dummy-error 1")
	push_error("dummy-error 2")
	push_error("dummy-error 3")

	await assert_signal(self).is_emitted("callback_processed")
	await get_tree().process_frame  # allow all events to process
	assert_int(_num_events).is_equal(2)

	# Wait for throttling window to expire.
	await get_tree().create_timer(1.1).timeout

	push_error("dummy-error 4")
	push_error("dummy-error 5")
	push_error("dummy-error 6")
	await assert_signal(self).is_emitted("callback_processed")
	await get_tree().process_frame  # allow all events to process
	assert_int(_num_events).is_equal(4)
