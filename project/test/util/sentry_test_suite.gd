class_name SentryTestSuite
extends GdUnitTestSuite
## Sentry test suite extensions for gdUnit4.

## Emitted after event was processed by before_send callback, and its JSON content stored in captured_events.
signal event_captured

var captured_events: Array[String]

var _saved_before_send: Callable


## Perform queries and assertions on JSON content.
func assert_json(json: Variant) -> JSONAssert:
	return JSONAssert.new(json)


## Capture event, expect it to be processed and return its JSON content.
func capture_event_and_get_json(event: SentryEvent) -> String:
	SentrySDK.capture_event(event)
	await assert_signal(self).is_emitted("event_captured")
	return captured_events[-1] if captured_events.size() > 0 else ""


## Expect an event to be processed and return its serialized JSON content.
func wait_for_captured_event_json() -> String:
	await assert_signal(self).is_emitted("event_captured")
	return captured_events[-1] if captured_events.size() > 0 else ""


func before() -> void:
	# NOTE: Make sure to call super() if overriding.
	# Save before send
	_saved_before_send = SentrySDK._get_before_send()


func after() -> void:
	# NOTE: Make sure to call super() if overriding.
	# Restore before send
	if _saved_before_send.is_valid():
		SentrySDK._set_before_send(_saved_before_send)


func before_test() -> void:
	# NOTE: Make sure to call super() if overriding.
	captured_events.clear()
	SentrySDK._set_before_send(_before_send)
	monitor_signals(self, false)


func after_test() -> void:
	# NOTE: Make sure to call super() if overriding.
	SentrySDK._unset_before_send()


func _before_send(event: SentryEvent) -> SentryEvent:
	captured_events.append(event.to_json())
	event_captured.emit()
	return null
