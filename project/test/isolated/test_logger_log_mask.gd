extends GdUnitTestSuite
## Errors should be auto-captured as Sentry Logs only when their type bit is
## set in "logger_log_mask".


var _captured: Array[Dictionary] = []


func before() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.enable_logs = true
		options.logger_log_mask = SentryOptions.MASK_ERROR
		options.before_send_log = _before_send_log

		# Make sure other limits are not interfering.
		options.logger_limits.events_per_frame = 88
		options.logger_limits.throttle_events = 88
		options.logger_limits.repeated_error_window_ms = 0
		options.logger_limits.throttle_window_ms = 0
	)


func _before_send_log(entry: SentryLog) -> SentryLog:
	_captured.append({
		"body": entry.body,
		"level": entry.level,
	})
	return entry


## With only MASK_ERROR set, push_error() should produce a Sentry Log,
## while push_warning() (whose bit is not in the mask) should be ignored.
func test_log_mask_captures_only_set_bits() -> void:
	push_error("captured-error")
	push_warning("ignored-warning")

	assert_int(_captured.size()).is_equal(1)
	assert_str(_captured[0].body).contains("captured-error")
	assert_int(_captured[0].level).is_equal(SentryLog.LOG_LEVEL_ERROR)
