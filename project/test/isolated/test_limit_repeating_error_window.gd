extends GdUnitTestSuite
## Test "repeated_error_window_ms" error logger limit.


signal callback_processed

var _count: int = 0


static func configure_options(options: SentryOptions) -> void:
	# Ignore duplicate errors within 1 second window.
	options.error_logger_limits.repeated_error_window_ms = 1000
	options.error_logger_limits.events_per_frame = 88
	options.error_logger_limits.throttle_events = 88


func before_test() -> void:
	SentrySDK._set_before_send(_before_send)


func _before_send(ev: SentryEvent) -> SentryEvent:
	_count += 1
	callback_processed.emit()
	return null


## Only one error should be logged within 1 second time window, and another one after 1 second passes.
func test_repeating_error_window_limit() -> void:
	push_error("dummy-error")
	push_error("dummy-error")
	await assert_signal(self).is_emitted("callback_processed")
	assert_int(_count).is_equal(1)

	await get_tree().create_timer(1.0).timeout

	push_error("dummy-error")
	push_error("dummy-error")
	await assert_signal(self).is_emitted("callback_processed")
	assert_int(_count).is_equal(2)
