extends SentryTestSuite
## Verifies telemetry stays on a single trace across scope operations.


func _trace_id(json: String) -> Variant:
	var data: Variant = JSON.parse_string(json)
	return data.get("contexts", {}).get("trace", {}).get("trace_id")


func test_scoped_event_shares_trace() -> void:
	var json_outside: String = await capture_event_and_get_json(SentrySDK.create_event())

	SentrySDK.with_scope(func(_scope: SentryScope) -> void:
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_in_scope: String = await wait_for_captured_event_json()

	assert_json(json_in_scope).describe("event captured in with_scope shares the trace of one captured outside") \
		.at("/contexts/trace/trace_id") \
		.is_equal(_trace_id(json_outside)) \
		.verify()


func test_nested_with_scope_keeps_trace() -> void:
	SentrySDK.with_scope(func(_outer: SentryScope) -> void:
		SentrySDK.capture_event(SentrySDK.create_event())

		SentrySDK.with_scope(func(_inner: SentryScope) -> void:
			SentrySDK.capture_event(SentrySDK.create_event())
			)
		)

	await wait_for_captured_event_json()
	var json_outer: String = captured_events[0]
	var json_inner: String = captured_events[1]

	assert_json(json_inner).describe("nested with_scope stays on the outer scope's trace") \
		.at("/contexts/trace/trace_id") \
		.is_equal(_trace_id(json_outer)) \
		.verify()


func test_scope_clear_keeps_trace() -> void:
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
