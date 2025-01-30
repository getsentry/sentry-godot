extends GdUnitTestSuite
## Test error logger throttling limits.


signal callback_processed

var _num_events: int = 0


static func configure_options(options: SentryOptions) -> void:
	# Allow only two errors to be logged as events within 2 second time window.
	options.error_logger_limits.throttle_events = 2
	options.error_logger_limits.throttle_window_ms = 10000
	# Make sure other limits are not applied.
	options.error_logger_limits.events_per_frame = 88
	options.error_logger_limits.repeated_error_window_ms = 0


func before_test() -> void:
	SentrySDK._set_before_send(_before_send)


func _before_send(_ev: SentryEvent) -> SentryEvent:
	_num_events += 1
	callback_processed.emit()
	return null


## Only two errors should be logged within the same time window.
func test_throttling_limits() -> void:
	push_error("dummy-error")
	push_error("dummy-error")
	push_error("dummy-error")
	await assert_signal(self).is_emitted("callback_processed")
	await assert_signal(self).is_emitted("callback_processed")
	await get_tree().create_timer(0.1).timeout
	assert_int(_num_events).is_equal(2)
