extends GdUnitTestSuite
## Test error logger throttling limits early in the app lifecycle.
##
## Note: During early app lifecycle special relaxed limits are in effect.


signal callback_processed

var _num_events: int = 0


func before() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.logger_limits.throttle_events = 2
		options.logger_limits.throttle_window_ms = 100000
		# Make sure other limits are not interfering.
		options.logger_limits.events_per_frame = 88
		options.logger_limits.repeated_error_window_ms = 0
		options.before_send = _before_send
	)


func _before_send(_ev: SentryEvent) -> SentryEvent:
	_num_events += 1
	callback_processed.emit()
	return null


## Only two errors should be logged within the assigned time window.
func test_throttling_limits() -> void:
	monitor_signals(self, false)

	# Ensure running early.
	assert_int(Engine.get_process_frames()).is_less(10)

	push_error("dummy-error 1")
	push_error("dummy-error 2")
	push_error("dummy-error 3")
	push_error("dummy-error 4")
	push_error("dummy-error 5")

	await get_tree().process_frame  # allow all events to process
	assert_int(_num_events).is_equal(5)
