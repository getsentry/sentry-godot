extends GdUnitTestSuite
## Events and breadcrumbs should be logged when "godot_logger.event_mask" and
## "godot_logger.breadcrumb_mask" are configured to include all categories.


signal callback_processed

var _num_events: int = 0


func before() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		var mask = SentryOptions.MASK_ERROR | SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER | SentryOptions.MASK_WARNING
		options.godot_logger.event_mask = mask
		options.godot_logger.breadcrumb_mask = mask

		# Make sure other limits are not interfering.
		options.godot_logger.limits.events_per_frame = 88
		options.godot_logger.limits.throttle_events = 88
		options.godot_logger.limits.repeated_error_window_ms = 0
		options.godot_logger.limits.throttle_window_ms = 0

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
