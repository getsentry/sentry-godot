extends SentryTestSuite
## Verifies events captured while a span is active are stamped with that span.


# TODO: widen the platform list as spans are implemented on other backends.
func before(_do_skip = OS.get_name() not in ["Windows", "Linux"],
		_skip_reason = "Spans are not implemented on this platform yet.") -> void:
	super()


func _span_id(json: String) -> Variant:
	var data: Variant = JSON.parse_string(json)
	return data.get("contexts", {}).get("trace", {}).get("span_id")


func _trace_id(json: String) -> Variant:
	var data: Variant = JSON.parse_string(json)
	return data.get("contexts", {}).get("trace", {}).get("trace_id")


func test_active_span_stamps_event() -> void:
	var json_before: String = await capture_event_and_get_json(SentrySDK.create_event())
	assert_object(SentrySDK.get_active_span()).is_null()

	var span := SentrySDK.start_span("test.active")
	assert_object(SentrySDK.get_active_span()).is_same(span)
	var json_in_span: String = await capture_event_and_get_json(SentrySDK.create_event())

	span.end()
	assert_object(SentrySDK.get_active_span()).is_null()
	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_span).describe("an active span stamps the event it encloses") \
		.at("/contexts/trace/span_id") \
		.is_not_equal(_span_id(json_before)) \
		.verify()

	assert_json(json_after).describe("events captured after the span ends carry the same id as before it started") \
		.at("/contexts/trace/span_id") \
		.is_equal(_span_id(json_before)) \
		.verify()


func test_events_under_one_span_share_span_id() -> void:
	var span := SentrySDK.start_span("test.shared")
	assert_object(SentrySDK.get_active_span()).is_same(span)
	var json_first: String = await capture_event_and_get_json(SentrySDK.create_event())
	var json_second: String = await capture_event_and_get_json(SentrySDK.create_event())
	span.end()

	assert_json(json_second).describe("events captured under the same span carry the same span_id") \
		.at("/contexts/trace/span_id") \
		.is_equal(_span_id(json_first)) \
		.verify()


func test_nested_span_stamps_and_restores_its_parent() -> void:
	var parent := SentrySDK.start_span("test.parent")
	assert_object(SentrySDK.get_active_span()).is_same(parent)
	var json_in_parent: String = await capture_event_and_get_json(SentrySDK.create_event())

	var child := SentrySDK.start_span("test.child")
	assert_object(SentrySDK.get_active_span()).is_same(child)
	var json_in_child: String = await capture_event_and_get_json(SentrySDK.create_event())

	child.end()
	assert_object(SentrySDK.get_active_span()).is_same(parent)
	var json_after_child: String = await capture_event_and_get_json(SentrySDK.create_event())
	parent.end()

	assert_json(json_in_child).describe("the innermost active span stamps the event") \
		.at("/contexts/trace/span_id") \
		.is_not_equal(_span_id(json_in_parent)) \
		.verify()

	assert_json(json_after_child).describe("ending a child span hands stamping back to its parent") \
		.at("/contexts/trace/span_id") \
		.is_equal(_span_id(json_in_parent)) \
		.verify()


func test_inactive_span_does_not_stamp() -> void:
	var json_before: String = await capture_event_and_get_json(SentrySDK.create_event())

	var span := SentrySDK.start_span("test.inactive", {}, null, false)
	assert_object(SentrySDK.get_active_span()).is_not_same(span)
	var json_alongside: String = await capture_event_and_get_json(SentrySDK.create_event())
	span.end()

	assert_json(json_alongside).describe("a span started with active=false leaves events unstamped") \
		.at("/contexts/trace/span_id") \
		.is_equal(_span_id(json_before)) \
		.verify()


func test_forked_scope_inherits_active_span() -> void:
	var span := SentrySDK.start_span("test.forked")
	SentrySDK.capture_event(SentrySDK.create_event())

	SentrySDK.with_scope(func(_scope: SentryScope) -> void:
		assert_object(SentrySDK.get_active_span()).is_same(span)
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	span.end()

	var json_outside: String = await wait_for_captured_event_json()
	var json_in_fork: String = await wait_for_captured_event_json()

	assert_json(json_in_fork).describe("a forked scope stamps with the span that was active when it forked") \
		.at("/contexts/trace/span_id") \
		.is_equal(_span_id(json_outside)) \
		.verify()


func test_scope_forked_after_span_ended_is_not_stamped() -> void:
	var span := SentrySDK.start_span("test.ended_before_fork")
	SentrySDK.capture_event(SentrySDK.create_event())
	span.end()

	SentrySDK.with_scope(func(_scope: SentryScope) -> void:
		assert_object(SentrySDK.get_active_span()).is_null()
		SentrySDK.capture_event(SentrySDK.create_event())
		)

	var json_in_span: String = await wait_for_captured_event_json()
	var json_in_fork: String = await wait_for_captured_event_json()

	assert_json(json_in_fork).describe("a scope forked after the span ended does not inherit its binding") \
		.at("/contexts/trace/span_id") \
		.is_not_equal(_span_id(json_in_span)) \
		.verify()


func test_scope_clear_drops_the_span() -> void:
	var span := SentrySDK.start_span("test.cleared")
	SentrySDK.capture_event(SentrySDK.create_event())

	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.clear()
		assert_object(SentrySDK.get_active_span()) \
			.override_failure_message("clear() must drop the active span") \
			.is_null()
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	span.end()

	var json_in_span: String = await wait_for_captured_event_json()
	var json_after_clear: String = await wait_for_captured_event_json()

	assert_json(json_after_clear).describe("later events are not stamped with previously cleared span") \
		.at("/contexts/trace/span_id") \
		.is_not_equal(_span_id(json_in_span)) \
		.verify()


func test_span_stays_on_the_current_trace() -> void:
	var json_before: String = await capture_event_and_get_json(SentrySDK.create_event())

	var span := SentrySDK.start_span("test.trace")
	var json_in_span: String = await capture_event_and_get_json(SentrySDK.create_event())

	span.end()
	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_span).describe("starting a span does not move events off the current trace") \
		.at("/contexts/trace/trace_id") \
		.is_equal(_trace_id(json_before)) \
		.verify()

	assert_json(json_after).describe("ending a span does not move later events off the current trace") \
		.at("/contexts/trace/trace_id") \
		.is_equal(_trace_id(json_before)) \
		.verify()


func test_with_span_forks_the_current_scope() -> void:
	SentrySDK.with_span("test.with_span_fork", func(_span: SentrySpan) -> void:
		SentrySDK.get_current_scope().set_tag("scoped", "in_span")
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_in_span: String = await wait_for_captured_event_json()

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_span).describe("set_tag() reaches the event captured inside with_span") \
		.at("/tags") \
		.must_contain("scoped", "in_span") \
		.verify()

	assert_json(json_after).describe("scope writes inside with_span do not outlive it") \
		.at("/tags") \
		.must_not_contain("scoped") \
		.verify()


func test_with_span_stamps_events_and_clears_the_slot() -> void:
	var json_before: String = await capture_event_and_get_json(SentrySDK.create_event())

	SentrySDK.with_span("test.with_span", func(span: SentrySpan) -> void:
		assert_object(SentrySDK.get_active_span()).is_same(span)
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	assert_object(SentrySDK.get_active_span()).is_null()

	var json_inside: String = await wait_for_captured_event_json()
	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_inside).describe("events captured inside with_span carry its span") \
		.at("/contexts/trace/span_id") \
		.is_not_equal(_span_id(json_before)) \
		.verify()

	assert_json(json_after).describe("events captured after with_span returns carry the same id as before it started") \
		.at("/contexts/trace/span_id") \
		.is_equal(_span_id(json_before)) \
		.verify()


func test_with_span_returns_the_callable_result() -> void:
	var result: Variant = SentrySDK.with_span("test.with_span_result", func(_span: SentrySpan) -> int:
		return 42
		)

	assert_int(result).is_equal(42)


func test_nested_with_span_stamps_with_the_inner_span() -> void:
	SentrySDK.with_span("test.with_span_outer", func(outer: SentrySpan) -> void:
		SentrySDK.capture_event(SentrySDK.create_event())
		SentrySDK.with_span("test.with_span_inner", func(_inner: SentrySpan) -> void:
			SentrySDK.capture_event(SentrySDK.create_event())
			)
		assert_object(SentrySDK.get_active_span()).is_same(outer)
		)

	var json_outer: String = await wait_for_captured_event_json()
	var json_inner: String = await wait_for_captured_event_json()

	assert_json(json_inner).describe("a nested with_span stamps events with the inner span") \
		.at("/contexts/trace/span_id") \
		.is_not_equal(_span_id(json_outer)) \
		.verify()


func test_with_span_inside_a_started_span_nests_under_it() -> void:
	var span := SentrySDK.start_span("test.manual_parent")
	SentrySDK.capture_event(SentrySDK.create_event())

	SentrySDK.with_span("test.with_span_child", func(_child: SentrySpan) -> void:
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	assert_object(SentrySDK.get_active_span()).is_same(span)

	SentrySDK.capture_event(SentrySDK.create_event())
	span.end()

	var json_in_parent: String = await wait_for_captured_event_json()
	var json_in_child: String = await wait_for_captured_event_json()
	var json_after_child: String = await wait_for_captured_event_json()

	assert_json(json_in_child).describe("with_span inside an active span stamps with its own span") \
		.at("/contexts/trace/span_id") \
		.is_not_equal(_span_id(json_in_parent)) \
		.verify()

	assert_json(json_after_child).describe("the enclosing span stamps again once with_span returns") \
		.at("/contexts/trace/span_id") \
		.is_equal(_span_id(json_in_parent)) \
		.verify()
