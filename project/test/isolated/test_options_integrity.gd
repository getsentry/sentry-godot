extends GutTest
## Verify that the options set in a configuration callback are correctly reflected in event objects.


signal callback_processed


static func configure_options(options: SentryOptions) -> void:
	options.release = "app@1.2.3"
	options.environment = "testing"


func before_each() -> void:
	SentrySDK._set_before_send(_before_send)


func _before_send(ev: SentryEvent) -> SentryEvent:
	assert_eq(ev.release, "app@1.2.3")
	assert_eq(ev.environment, "testing")
	callback_processed.emit()
	return null


## Verify that the options are correctly propagated to event objects.
func test_options_integrity() -> void:
	var ev := SentrySDK.create_event()
	watch_signals(self)
	SentrySDK.capture_event(ev)
	await wait_for_signal(callback_processed, 2.0)
	assert_signal_emitted(callback_processed)
