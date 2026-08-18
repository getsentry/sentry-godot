extends SentryTestSuite
## Verifies events captured while a span is active are stamped with that span.


# TODO: widen the platform list as spans are implemented on other backends.
func before(_do_skip = OS.get_name() not in ["Windows", "Linux", "Web"],
		_skip_reason = "Spans are not implemented on this platform yet.") -> void:
	super()


func after_test() -> void:
	super()
	SentrySDK.get_current_scope().clear()


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


func test_span_inherits_the_scope_it_started_from() -> void:
	SentrySDK.get_current_scope().set_tag("before_span", "current")

	var span := SentrySDK.start_span("test.inherits_scope")
	var json_in_span: String = await capture_event_and_get_json(SentrySDK.create_event())
	span.end()

	assert_json(json_in_span).describe("a span carries scope data written before it started") \
		.at("/tags") \
		.must_contain("before_span", "current") \
		.verify()


func test_scope_write_inside_a_span_does_not_outlive_it() -> void:
	var span := SentrySDK.start_span("test.scope_write")
	SentrySDK.get_current_scope().set_tag("spanned", "in_span")
	SentrySDK.capture_event(SentrySDK.create_event())
	span.end()

	var json_in_span: String = await wait_for_captured_event_json()
	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_span).describe("set_tag() on the current scope reaches the event captured inside the span") \
		.at("/tags") \
		.must_contain("spanned", "in_span") \
		.verify()

	assert_json(json_after).describe("scope writes made while a span was active do not outlive it") \
		.at("/tags") \
		.must_not_contain("spanned") \
		.verify()


func test_ending_spans_leaves_the_scope_stack_where_it_started() -> void:
	SentrySDK.get_current_scope().set_tag("root", "root_scope")

	for i in 4:
		var span := SentrySDK.start_span("test.balanced_" + str(i))
		SentrySDK.get_current_scope().set_tag("span_" + str(i), "fork")
		span.end()

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_after).describe("the scope in effect after a batch of spans is the one they started from") \
		.at("/tags") \
		.must_contain("root", "root_scope") \
		.must_not_contain("span_0") \
		.must_not_contain("span_1") \
		.must_not_contain("span_2") \
		.must_not_contain("span_3") \
		.verify()


func test_explicit_parent_inherits_the_parents_scope() -> void:
	var parent := SentrySDK.start_span("test.explicit_parent")
	SentrySDK.get_current_scope().set_tag("parent_tag", "parent")

	var unrelated := SentrySDK.start_span("test.unrelated")
	SentrySDK.get_current_scope().set_tag("unrelated_tag", "unrelated")

	var child := SentrySDK.start_span("test.explicit_child", {}, parent)
	SentrySDK.capture_event(SentrySDK.create_event())
	child.end()
	unrelated.end()
	parent.end()

	var json_in_child: String = await wait_for_captured_event_json()

	assert_json(json_in_child).describe("an explicitly parented span forks the parent's scope rather than the caller's") \
		.at("/tags") \
		.must_contain("parent_tag", "parent") \
		.must_not_contain("unrelated_tag") \
		.verify()


func test_ended_span_refuses_a_child() -> void:
	var json_before: String = await capture_event_and_get_json(SentrySDK.create_event())

	var parent := SentrySDK.start_span("test.ended_parent")
	var json_in_parent: String = await capture_event_and_get_json(SentrySDK.create_event())
	parent.end()

	var orphan := SentrySDK.start_span("test.orphan", {}, parent)

	assert_object(orphan) \
		.override_failure_message("an ended span must yield a no-op span instead of null") \
		.is_not_null()
	assert_object(SentrySDK.get_active_span()) \
		.override_failure_message("a no-op span must become active so its children inherit it") \
		.is_same(orphan)

	var json_in_orphan: String = await capture_event_and_get_json(SentrySDK.create_event())
	orphan.end()

	assert_json(json_in_orphan).describe("a no-op span leaves its events on the propagation context instead of the ended parent") \
		.at("/contexts/trace/span_id") \
		.is_not_equal(_span_id(json_in_parent)) \
		.is_equal(_span_id(json_before)) \
		.verify()

	assert_json(json_in_orphan).describe("a no-op span stays on the trace it was started from") \
		.at("/contexts/trace/trace_id") \
		.is_equal(_trace_id(json_before)) \
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
