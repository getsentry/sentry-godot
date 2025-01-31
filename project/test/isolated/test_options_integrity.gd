extends GdUnitTestSuite
## Verify that the options set in a configuration callback are correctly reflected in event objects.


signal callback_processed


static func configure_options(options: SentryOptions) -> void:
	options.release = "1.2.3"
	options.environment = "testing"


func before_test() -> void:
	SentrySDK._set_before_send(_before_send)


func _before_send(ev: SentryEvent) -> SentryEvent:
	assert_str(ev.release).is_equal("1.2.3")
	assert_str(ev.environment).is_equal("testing")
	callback_processed.emit()
	return null


## Verify that the options are correctly propagated to event objects.
func test_options_integrity() -> void:
	var ev := SentrySDK.create_event()
	var monitor := monitor_signals(self, false)
	SentrySDK.capture_event(ev)
	await assert_signal(monitor).is_emitted("callback_processed")
