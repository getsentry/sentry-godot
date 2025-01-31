extends GdUnitTestSuite
## Events and breadcrumbs should be logged when "error_logger_event_mask" and
## "error_logger_breadcrumb_mask" are configured to include all categories.


signal callback_processed

var _num_events: int = 0


static func configure_options(options: SentryOptions) -> void:
    var mask = SentryOptions.MASK_ERROR | SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER | SentryOptions.MASK_WARNING
    options.error_logger_event_mask = mask
    options.error_logger_breadcrumb_mask = mask

    # Make sure other limits are not interfering.
    options.error_logger_limits.events_per_frame = 88
    options.error_logger_limits.throttle_events = 88
    options.error_logger_limits.repeated_error_window_ms = 0
    options.error_logger_limits.throttle_window_ms = 0


func before_test() -> void:
    SentrySDK._set_before_send(_before_send)


func _before_send(_ev: SentryEvent) -> SentryEvent:
    _num_events += 1
    callback_processed.emit()
    return null


## Both events or breadcrumbs should be logged for error and warning.
## TODO: can't verify breadcrumbs yet, maybe later.
func test_event_and_breadcrumb_masks() -> void:
    push_error("dummy-error")
    push_warning("dummy-warning")
    await assert_signal(self).is_emitted("callback_processed")

    await get_tree().create_timer(0.1).timeout
    assert_int(_num_events).is_equal(2)
