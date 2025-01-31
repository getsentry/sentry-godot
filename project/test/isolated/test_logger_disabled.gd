extends GdUnitTestSuite
## Events shouldn't be logged for errors when logger is disabled.


signal callback_processed

var _num_events: int = 0


static func configure_options(options: SentryOptions) -> void:
    options.error_logger_enabled = false

    ## Ensure no interference from logger limits.
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


func test_event_and_breadcrumb_masks() -> void:
    push_error("dummy-error")
    push_warning("dummy-warning")

    await assert_signal(self).is_not_emitted("callback_processed")

    assert_int(_num_events).is_equal(0)
