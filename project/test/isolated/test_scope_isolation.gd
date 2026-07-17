extends SentryTestSuite
## Verifies scope isolation for `SentrySDK.with_scope()`.
##
## A write to the local scope must only affect events captured inside the block,
## must not leak back into the global scope afterwards, and top-level writes must
## still reach the global scope even when made from inside the block.


func test_with_scope_set_context() -> void:
	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_context("scene", {"name": "Dungeon", "depth": 3})
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_in_scope: String = await wait_for_captured_event_json()

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_scope).describe("set_context() reaches the in-scope event") \
		.at("/contexts/scene") \
		.is_object() \
		.must_contain("name", "Dungeon") \
		.must_contain("depth", 3) \
		.verify()

	assert_json(json_after).describe("set_context() does not leak past with_scope") \
		.at("/") \
		.must_not_contain("/contexts/scene") \
		.verify()


func test_with_scope_set_tag() -> void:
	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_tag("scoped", "in_scope")
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_in_scope: String = await wait_for_captured_event_json()

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_scope).describe("set_tag() reaches the in-scope event") \
		.at("/tags") \
		.must_contain("scoped", "in_scope") \
		.verify()

	assert_json(json_after).describe("set_tag() does not leak past with_scope") \
		.at("/tags") \
		.must_not_contain("scoped") \
		.verify()


func test_with_scope_set_user() -> void:
	var scoped_user := SentryUser.new()
	scoped_user.id = "player_scope"
	scoped_user.username = "ScopedPlayer"

	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_user(scoped_user)
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_in_scope: String = await wait_for_captured_event_json()

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_scope).describe("set_user() reaches the in-scope event") \
		.at("/user") \
		.is_object() \
		.must_contain("id", "player_scope") \
		.must_contain("username", "ScopedPlayer") \
		.verify()

	assert_json(json_after).describe("set_user() does not leak past with_scope") \
		.at("/") \
		.must_not_contain("/user/id", "player_scope") \
		.verify()


func test_with_scope_set_level() -> void:
	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_level(SentrySDK.LEVEL_WARNING)
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_in_scope: String = await wait_for_captured_event_json()

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_scope).describe("set_level() reaches the in-scope event") \
		.at("/") \
		.must_contain("level", "warning") \
		.verify()

	assert_json(json_after).describe("set_level() does not leak past with_scope") \
		.at("/") \
		.must_not_contain("/level", "warning") \
		.verify()


func test_with_scope_set_fingerprint() -> void:
	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_fingerprint(PackedStringArray(["scope-group", "scope-key"]))
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_in_scope: String = await wait_for_captured_event_json()

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_scope).describe("set_fingerprint() reaches the in-scope event") \
		.at("/fingerprint") \
		.is_array() \
		.has_size(2) \
		.must_contain("/0", "scope-group") \
		.must_contain("/1", "scope-key") \
		.verify()

	assert_json(json_after).describe("set_fingerprint() does not leak past with_scope") \
		.at("/") \
		.must_not_contain("/fingerprint/0", "scope-group") \
		.verify()


func test_with_scope_add_breadcrumb() -> void:
	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.add_breadcrumb(SentryBreadcrumb.create("scoped crumb"))
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json_in_scope: String = await wait_for_captured_event_json()

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_in_scope).describe("add_breadcrumb() reaches the in-scope event") \
		.at("/breadcrumbs/") \
		.is_array() \
		.with_objects() \
		.containing("message", "scoped crumb") \
		.exactly(1)

	assert_json(json_after).describe("add_breadcrumb() does not leak past with_scope") \
		.either() \
			.at("/") \
			.must_not_contain("/breadcrumbs") \
		.or_else() \
			.at("/breadcrumbs") \
			.is_null() \
		.or_else() \
			.at("/breadcrumbs/") \
			.with_objects() \
			.containing("message", "scoped crumb") \
			.must_selected(0) \
		.end() \
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


func test_with_scope_top_level_write() -> void:
	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		SentrySDK.set_tag("global_inside", "inside")
		)

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_after).describe("top-level write from inside with_scope reaches the global scope") \
		.at("/tags") \
		.must_contain("global_inside", "inside") \
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


func test_with_scope_capture_message() -> void:
	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_tag("scoped", "in_scope")
		SentrySDK.capture_message("scoped message", SentrySDK.LEVEL_WARNING)
		)
	var json_in_scope: String = await wait_for_captured_event_json()

	SentrySDK.capture_message("global message", SentrySDK.LEVEL_WARNING)
	var json_after: String = await wait_for_captured_event_json()

	assert_json(json_in_scope).describe("Top-level capture_message() goes through current scope") \
		.at("/tags") \
		.must_contain("scoped", "in_scope") \
		.verify()

	assert_json(json_after).describe("capture_message() does not carry the popped scope") \
		.at("/tags") \
		.must_not_contain("scoped") \
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
