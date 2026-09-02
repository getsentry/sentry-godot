extends SentryTestSuite
## Verifies telemetry stays on a single trace across scope operations, and moves off it when a new trace is started.
##
## TODO: drop the skips when Cocoa gains scope support.


func _trace_id(json: String) -> Variant:
	var data: Variant = JSON.parse_string(json)
	return data.get("contexts", {}).get("trace", {}).get("trace_id")


func test_scoped_event_shares_trace(_do_skip = OS.get_name() in ["macOS", "iOS"],
		_skip_reason = "Scopes are not implemented on this platform yet.") -> void:
	var json_outside: String = await capture_event_and_get_json(SentrySDK.create_event())

	SentrySDK.with_scope(func(_scope: SentryScope) -> void:
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_in_scope: String = await wait_for_captured_event_json()

	assert_json(json_in_scope).describe("event captured in with_scope shares the trace of one captured outside") \
		.at("/contexts/trace/trace_id") \
		.is_equal(_trace_id(json_outside)) \
		.verify()


func test_nested_with_scope_keeps_trace(_do_skip = OS.get_name() in ["macOS", "iOS"],
		_skip_reason = "Scopes are not implemented on this platform yet.") -> void:
	SentrySDK.with_scope(func(_outer: SentryScope) -> void:
		SentrySDK.capture_event(SentrySDK.create_event())

		SentrySDK.with_scope(func(_inner: SentryScope) -> void:
			SentrySDK.capture_event(SentrySDK.create_event())
			)
		)

	var json_outer: String = await wait_for_captured_event_json()
	var json_inner: String = await wait_for_captured_event_json()

	assert_json(json_inner).describe("nested with_scope stays on the outer scope's trace") \
		.at("/contexts/trace/trace_id") \
		.is_equal(_trace_id(json_outer)) \
		.verify()


func test_scope_clear_keeps_trace(_do_skip = OS.get_name() in ["macOS", "iOS"],
		_skip_reason = "Scopes are not implemented on this platform yet.") -> void:
	var json_before: String = await capture_event_and_get_json(SentrySDK.create_event())

	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.clear()
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_after_clear: String = await wait_for_captured_event_json()

	assert_json(json_after_clear).describe("clear() does not change the trace") \
		.at("/contexts/trace/trace_id") \
		.is_equal(_trace_id(json_before)) \
		.verify()


func test_start_new_trace_replaces_the_trace() -> void:
	var json_before: String = await capture_event_and_get_json(SentrySDK.create_event())

	SentrySDK.start_new_trace()
	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_after).describe("start_new_trace() moves later events onto another trace") \
		.at("/contexts/trace/trace_id") \
		.is_not_equal(_trace_id(json_before)) \
		.verify()


func test_events_after_start_new_trace_share_trace() -> void:
	SentrySDK.start_new_trace()

	var json_first: String = await capture_event_and_get_json(SentrySDK.create_event())
	var json_second: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_second).describe("events captured after start_new_trace() share one trace") \
		.at("/contexts/trace/trace_id") \
		.is_equal(_trace_id(json_first)) \
		.verify()
