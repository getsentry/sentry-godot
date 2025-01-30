extends GdUnitTestSuite
## Test error logger throttling limit.


var _count: int = 0


static func configure_options(options: SentryOptions) -> void:
	options.error_logger_limits.throttle_events = 2
	options.error_logger_limits.throttle_window_ms = 10000
	options.error_logger_limits.events_per_frame = 20


func before_test() -> void:
	SentrySDK._set_before_send(_before_send)


func _before_send(ev: SentryEvent) -> SentryEvent:
	_count += 1
	return null


## Only two errors should be logged within the same time window.
func test_error_logger() -> void:
	push_error("dummy-error")
	await get_tree().process_frame
	push_error("dummy-error")
	await get_tree().process_frame
	push_error("dummy-error")
	await get_tree().process_frame
	assert_int(_count).is_equal(2)
