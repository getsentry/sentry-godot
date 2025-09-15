class_name SentryTestSuite
extends GdUnitTestSuite
## Sentry test suite extensions for gdUnit4.
##
## The default `before_send` handler collects captured event json content in
## `captured_events` array in the chronological order. The array is cleared
## before each test.
## The `before_send` handler is reassigned before each test, and unset after each test.
## To override the default `before_send` handler, simply set a new one in a test body.


## Emitted after event was processed by before_send callback, and its JSON content stored in captured_events.
signal event_captured

var captured_events: Array[String]


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
	if not SentrySDK.is_enabled():
		SentrySDK.init(func(options: SentryOptions) -> void:
			options.logger_messages_as_breadcrumbs = false  # this may interfere with our tests
		)


func after() -> void:
	# NOTE: Make sure to call super() if overriding.
	pass


func before_test() -> void:
	# NOTE: Make sure to call super() if overriding.
	captured_events.clear()
	SentrySDK._set_before_send(_before_send)
	monitor_signals(self, false)


func after_test() -> void:
	# NOTE: Make sure to call super() if overriding.
	SentrySDK._unset_before_send()  # ignore events between tests


func _before_send(event: SentryEvent) -> SentryEvent:
	captured_events.append(event.to_json())
	event_captured.emit()
	return null
