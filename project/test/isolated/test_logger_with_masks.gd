extends GdUnitTestSuite
## Events and breadcrumbs should be logged when "logger_event_mask" and
## "logger_breadcrumb_mask" are configured to include all categories.


signal callback_processed

var _num_events: int = 0


func before() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		var mask = SentryOptions.MASK_ERROR | SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER | SentryOptions.MASK_WARNING
		options.logger_event_mask = mask
		options.logger_breadcrumb_mask = mask

		# Make sure other limits are not interfering.
		options.logger_limits.events_per_frame = 88
		options.logger_limits.throttle_events = 88
		options.logger_limits.repeated_error_window_ms = 0
		options.logger_limits.throttle_window_ms = 0

		options.before_send = _before_send
	)


func _before_send(ev: SentryEvent) -> SentryEvent:
	if ev.is_crash():
		# Likely processing previous crash.
		return ev
	_num_events += 1
	callback_processed.emit()
	return null


## Both events or breadcrumbs should be logged for error and warning.
## TODO: can't verify breadcrumbs yet, maybe later.
func test_event_and_breadcrumb_masks() -> void:
	var monitor := monitor_signals(self, false)
	push_error("dummy-error")
	push_warning("dummy-warning")
	await assert_signal(monitor).is_emitted("callback_processed")

	await get_tree().create_timer(0.1).timeout
	assert_int(_num_events).is_equal(2)
