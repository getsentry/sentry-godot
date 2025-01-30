extends GdUnitTestSuite
## Test error logger limit "events_per_frame".


var _count: int = 0


static func configure_options(options: SentryOptions) -> void:
	## Only one error is allowed to be logged per processed frame.
	options.error_logger_limits.events_per_frame = 1


func before_test() -> void:
	SentrySDK._set_before_send(_before_send)


func _before_send(ev: SentryEvent) -> SentryEvent:
	_count += 1
	return null


## Only one error should be logged.
func test_error_logger() -> void:
	push_error("dummy-error")
	push_error("dummy-error")
	await get_tree().process_frame
	assert_int(_count).is_equal(1)
