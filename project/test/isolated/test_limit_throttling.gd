extends GutTest
## Test error logger throttling limits.


signal callback_processed

var _num_events: int = 0


static func configure_options(options: SentryOptions) -> void:
	# Allow only two errors to be logged as events within 1 second time window.
	options.logger_limits.throttle_events = 2
	options.logger_limits.throttle_window_ms = 1000
	# Make sure other limits are not interfering.
	options.logger_limits.events_per_frame = 88
	options.logger_limits.repeated_error_window_ms = 0


func before_each() -> void:
	SentrySDK._set_before_send(_before_send)


func _before_send(_ev: SentryEvent) -> SentryEvent:
	_num_events += 1
	callback_processed.emit()
	return null


## Only two errors should be logged within the assigned time window.
func test_throttling_limits() -> void:
	watch_signals(self)
	push_error("dummy-error")
	push_error("dummy-error")
	push_error("dummy-error")
	await wait_for_signal(callback_processed, 2.0)
	assert_signal_emitted(callback_processed)
	await wait_for_signal(callback_processed, 2.0)
	assert_signal_emitted(callback_processed)
	await get_tree().create_timer(0.1).timeout
	assert_eq(_num_events, 2)

	# Wait for throttling window to expire.
	await get_tree().create_timer(1.0).timeout

	push_error("dummy-error")
	push_error("dummy-error")
	push_error("dummy-error")
	await wait_for_signal(callback_processed, 2.0)
	assert_signal_emitted(callback_processed)
	await wait_for_signal(callback_processed, 2.0)
	assert_signal_emitted(callback_processed)
	await get_tree().create_timer(0.1).timeout
	assert_eq(_num_events, 4)
