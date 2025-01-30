extends GdUnitTestSuite
## Verify that the options are correctly set in a configuration callback
## and are reflected in event objects.


signal callback_processed


static func configure_options(options: SentryOptions) -> void:
	options.release = "1.2.3"
	options.environment = "testing"


func before_test() -> void:
	SentrySDK._set_before_send(_before_send)


## Verify that the options are correctly propagated to event objects.
func test_options_integrity() -> void:
	var ev := SentrySDK.create_event()
	SentrySDK.capture_event(ev)
	assert_signal(self).is_emitted("callback_processed")


func _before_send(ev: SentryEvent) -> SentryEvent:
	assert_str(ev.release).is_equal("1.2.3")
	assert_str(ev.environment).is_equal("testing")
	callback_processed.emit()
	return null
