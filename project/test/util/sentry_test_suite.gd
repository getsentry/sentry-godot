class_name SentryTestSuite
extends GdUnitTestSuite
## Sentry test suite extensions for gdUnit4.
##
## Base test suite class for Sentry SDK testing that automatically initializes the SDK
## and provides utilities for capturing and analyzing event JSON content.
##
## The default "before_send" handler intercepts all events and stores their JSON
## representations in the "captured_events" array in chronological order. This array
## is automatically cleared before each test case runs.
##
## The "before_send" handler is reassigned before each test case and removed afterward
## to ensure clean runs. To customize event handling, simply assign a new
## "before_send" handler within your test method.


## Emitted after event was processed by before_send callback, and its JSON content stored in captured_events.
signal event_captured

var captured_events: Array[String]

## Position of the next event to be returned by wait_for_captured_event_json().
var _next_captured_event := 0


## Perform queries and assertions on JSON content.
func assert_json(json: Variant) -> JSONAssert:
	return JSONAssert.new(json)


## Capture event, expect it to be processed and return its JSON content.
func capture_event_and_get_json(event: SentryEvent) -> String:
	SentrySDK.capture_event(event)
	return await wait_for_captured_event_json()


## Expect an event to be processed and return its serialized JSON content.
## Consecutive calls hand out captured events in capture order.
func wait_for_captured_event_json() -> String:
	if _next_captured_event >= captured_events.size():
		await await_signal_on(self, "event_captured")
	if _next_captured_event >= captured_events.size():
		return ""
	var json: String = captured_events[_next_captured_event]
	_next_captured_event += 1
	return json


func before() -> void:
	# NOTE: Make sure to call super() if overriding.
	if not SentrySDK.is_enabled():
		init_sdk()


## Override this method in isolated tests to customize SDK initialization.
##
## NOTE: An isolated test suite executes in a separate run and typically
## customizes SDK options for specific testing scenarios.
func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		# Disable message breadcrumbs to avoid interfering with normal testing.
		options.godot_logger.breadcrumb_mask &= ~SentryOptions.MASK_MESSAGE
	)


func after() -> void:
	# NOTE: Make sure to call super() if overriding.
	pass


func before_test() -> void:
	# NOTE: Make sure to call super() if overriding.
	captured_events.clear()
	_next_captured_event = 0
	SentrySDK._set_before_send(_before_send)
	monitor_signals(self, false)


func after_test() -> void:
	# NOTE: Make sure to call super() if overriding.
	SentrySDK._unset_before_send()  # ignore events between tests


func _before_send(event: SentryEvent) -> SentryEvent:
	if event.is_crash():
		# Likely processing previous crash.
		return event
	var json := event.to_json()
	if OS.get_thread_caller_id() == OS.get_main_thread_id():
		_record_captured_event(json)
	else:
		# Defer: Node signals can only be emitted on the owning (i.e. main) thread.
		_record_captured_event.call_deferred(json)
	return null


func _record_captured_event(json: String) -> void:
	captured_events.append(json)
	event_captured.emit()
