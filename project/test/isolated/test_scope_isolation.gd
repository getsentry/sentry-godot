extends SentryTestSuite
## Verifies scope isolation for `SentrySDK.with_scope()`.
##
## A write to the local scope must only affect events captured inside the block,
## must not leak back into the global scope afterwards, and top-level writes must
## still reach the global scope even when made from inside the block.


func test_with_scope_write_isolation() -> void:
	SentrySDK.set_tag("global_before", "before")

	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_tag("scoped", "in_scope")
		SentrySDK.set_tag("global_inside", "inside")
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_in_scope: String = await wait_for_captured_event_json()

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_scope).describe("in-scope event carries local and global tags") \
		.at("/tags") \
		.must_contain("scoped", "in_scope") \
		.must_contain("global_before", "before") \
		.must_contain("global_inside", "inside") \
		.verify()

	assert_json(json_after).describe("local write must not leak past with_scope") \
		.at("/tags") \
		.must_not_contain("scoped") \
		.verify()

	assert_json(json_after).describe("top-level writes reach the global scope") \
		.at("/tags") \
		.must_contain("global_before", "before") \
		.must_contain("global_inside", "inside") \
		.verify()


func test_scope_clear() -> void:
	SentrySDK.set_tag("global_tag", "global")

	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_tag("before_clear", "value")
		scope.clear()
		scope.set_tag("after_clear", "value")
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("clear() drops local scope data written before it") \
		.at("/tags") \
		.must_not_contain("before_clear") \
		.verify()

	assert_json(json).describe("scope stays usable after clear()") \
		.at("/tags") \
		.must_contain("after_clear", "value") \
		.verify()

	assert_json(json).describe("clear() leaves the global scope intact") \
		.at("/tags") \
		.must_contain("global_tag", "global") \
		.verify()


func test_nested_with_scope() -> void:
	SentrySDK.with_scope(func(outer: SentryScope) -> void:
		outer.set_tag("outer_tag", "outer")

		SentrySDK.with_scope(func(inner: SentryScope) -> void:
			inner.set_tag("inner_tag", "inner")
			SentrySDK.capture_event(SentrySDK.create_event())
			)

		SentrySDK.capture_event(SentrySDK.create_event())
		)

	await wait_for_captured_event_json()
	var json_inner: String = captured_events[0]
	var json_outer: String = captured_events[1]

	assert_json(json_inner).describe("nested scope inherits the outer scope's data") \
		.at("/tags") \
		.must_contain("outer_tag", "outer") \
		.verify()

	assert_json(json_inner).describe("nested scope carries its own writes") \
		.at("/tags") \
		.must_contain("inner_tag", "inner") \
		.verify()

	assert_json(json_outer).describe("nested writes don't leak back to the outer scope") \
		.at("/tags") \
		.must_not_contain("inner_tag") \
		.verify()

	assert_json(json_outer).describe("outer scope keeps its own data") \
		.at("/tags") \
		.must_contain("outer_tag", "outer") \
		.verify()


func test_get_current_scope_not_null() -> void:
	assert_object(SentrySDK.get_current_scope()) \
		.override_failure_message("get_current_scope() returns a non-null current scope outside any with_scope block") \
		.is_not_null()


func test_get_current_scope_with_nesting() -> void:
	var initial: SentryScope = SentrySDK.get_current_scope()

	SentrySDK.with_scope(func(outer: SentryScope) -> void:
		assert_object(SentrySDK.get_current_scope()).is_same(outer)
		assert_object(SentrySDK.get_current_scope()).is_not_same(initial)

		SentrySDK.with_scope(func(inner: SentryScope) -> void:
			assert_object(SentrySDK.get_current_scope()).is_same(inner)
			assert_object(SentrySDK.get_current_scope()).is_not_same(outer)
			)

		assert_object(SentrySDK.get_current_scope()).is_same(outer)
		)

	assert_object(SentrySDK.get_current_scope()).is_same(initial)
