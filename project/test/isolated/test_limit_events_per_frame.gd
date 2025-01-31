extends GdUnitTestSuite
## Test "events_per_frame" error logger limit.


signal callback_processed

var _num_events: int = 0


static func configure_options(options: SentryOptions) -> void:
	# Only one error is allowed to be logged as event per processed frame.
	options.error_logger_limits.events_per_frame = 1
	# Make sure other limits are not interfering.
	options.error_logger_limits.repeated_error_window_ms = 0
	options.error_logger_limits.throttle_events = 88


func before_test() -> void:
	SentrySDK._set_before_send(_before_send)


func _before_send(_ev: SentryEvent) -> SentryEvent:
	_num_events += 1
	callback_processed.emit()
	return null


## Only one error should be logged within 1 processed frame.
func test_events_per_frame_limit() -> void:
	push_error("dummy-error")
	push_error("dummy-error")
	push_error("dummy-error")
	assert_signal(self).is_emitted("callback_processed")
	await get_tree().create_timer(0.1).timeout
	assert_int(_num_events).is_equal(1)
