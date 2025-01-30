extends GdUnitTestSuite
## Events and breadcrumbs should not be logged when both "error_logger_event_mask"
## and "error_logger_breadcrumb_mask" are set to zero.


signal callback_processed

var _num_events: int = 0


static func configure_options(options: SentryOptions) -> void:
    options.error_logger_event_mask = 0
    options.error_logger_breadcrumb_mask = 0


func before_test() -> void:
    SentrySDK._set_before_send(_before_send)


func _before_send(_ev: SentryEvent) -> SentryEvent:
    _num_events += 1
    callback_processed.emit()
    return null


## No events or breadcrumbs should be logged for errors.
## TODO: can't verify breadcrumbs yet, maybe later.
func test_event_and_breadcrumb_masks() -> void:
    push_error("dummy-error")
    push_warning("dummy-warning")
    await assert_signal(self).is_not_emitted("callback_processed")

    await get_tree().create_timer(0.1).timeout
    assert_int(_num_events).is_equal(0)
