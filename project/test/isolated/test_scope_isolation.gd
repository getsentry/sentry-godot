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
