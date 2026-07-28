extends SentryTestSuite
## Verifies scope isolation for `SentrySDK.with_scope()`.


# TODO: widen the platform list as scopes are implemented on other backends.
func before(_do_skip = OS.get_name() not in ["Windows", "Linux", "Android", "Web"],
		_skip_reason = "Scopes are not implemented on this platform yet.") -> void:
	super()


func test_scoped_set_context() -> void:
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


func test_scoped_set_tag() -> void:
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


func test_scoped_set_user() -> void:
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


func test_scoped_set_level() -> void:
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


func test_scoped_set_fingerprint() -> void:
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


func test_scoped_add_breadcrumb() -> void:
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

	var cleared_user := SentryUser.new()
	cleared_user.id = "player_cleared"

	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_context("cleared_scene", {"name": "Dungeon"})
		scope.set_tag("before_clear", "value")
		scope.set_user(cleared_user)
		scope.set_level(SentrySDK.LEVEL_WARNING)
		scope.set_fingerprint(PackedStringArray(["cleared-group"]))
		scope.add_breadcrumb(SentryBreadcrumb.create("cleared crumb"))
		scope.clear()
		scope.set_tag("after_clear", "value")
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("clear() drops the context written before it") \
		.at("/") \
		.must_not_contain("/contexts/cleared_scene") \
		.verify()

	assert_json(json).describe("clear() drops the tag written before it") \
		.at("/tags") \
		.must_not_contain("before_clear") \
		.verify()

	assert_json(json).describe("clear() drops the user written before it") \
		.at("/") \
		.must_not_contain("/user/id", "player_cleared") \
		.verify()

	assert_json(json).describe("clear() drops the level written before it") \
		.at("/") \
		.must_not_contain("/level", "warning") \
		.verify()

	assert_json(json).describe("clear() drops the fingerprint written before it") \
		.at("/") \
		.must_not_contain("/fingerprint/0", "cleared-group") \
		.verify()

	assert_json(json).describe("clear() drops the breadcrumb added before it") \
		.either() \
			.at("/") \
			.must_not_contain("/breadcrumbs") \
		.or_else() \
			.at("/breadcrumbs") \
			.is_null() \
		.or_else() \
			.at("/breadcrumbs/") \
			.with_objects() \
			.containing("message", "cleared crumb") \
			.must_selected(0) \
		.end() \
		.verify()

	assert_json(json).describe("scope stays usable after clear()") \
		.at("/tags") \
		.must_contain("after_clear", "value") \
		.verify()

	assert_json(json).describe("clear() leaves the global scope intact") \
		.at("/tags") \
		.must_contain("global_tag", "global") \
		.verify()


func test_scoped_capture_message() -> void:
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


func test_with_scope_top_level_write() -> void:
	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		SentrySDK.set_tag("global_inside", "inside")
		)

	var json_after: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_after).describe("top-level write from inside with_scope reaches the global scope") \
		.at("/tags") \
		.must_contain("global_inside", "inside") \
		.verify()


func test_with_scope_returns_callable_result() -> void:
	var result: Variant = SentrySDK.with_scope(func(_scope: SentryScope) -> String:
		return "callable result"
		)

	assert_str(result) \
		.override_failure_message("with_scope() hands back what the callable returned") \
		.is_equal("callable result")


func test_nested_with_scope() -> void:
	SentrySDK.with_scope(func(outer: SentryScope) -> void:
		outer.set_tag("outer_tag", "outer")

		SentrySDK.with_scope(func(inner: SentryScope) -> void:
			inner.set_tag("inner_tag", "inner")
			SentrySDK.capture_event(SentrySDK.create_event())
			)

		SentrySDK.capture_event(SentrySDK.create_event())
		)

	var json_inner: String = await wait_for_captured_event_json()
	var json_outer: String = await wait_for_captured_event_json()

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


func test_get_current_scope_is_thread_local() -> void:
	var main_scope: SentryScope = SentrySDK.get_current_scope()

	var thread := Thread.new()
	thread.start(func() -> SentryScope:
		return SentrySDK.get_current_scope()
		)
	var worker_scope: SentryScope = thread.wait_to_finish()

	assert_object(worker_scope) \
		.override_failure_message("worker thread gets a current scope of its own") \
		.is_not_null()

	assert_object(worker_scope) \
		.override_failure_message("worker thread doesn't share the main thread's current scope") \
		.is_not_same(main_scope)


func test_get_current_scope_enriches_later_events() -> void:
	SentrySDK.get_current_scope().set_tag("current_tag", "current")

	var json_first: String = await capture_event_and_get_json(SentrySDK.create_event())
	var json_second: String = await capture_event_and_get_json(SentrySDK.create_event())

	# The current scope outlives the test, so leave it as it was found.
	SentrySDK.get_current_scope().clear()

	assert_json(json_first).describe("current scope enriches an event captured after the write") \
		.at("/tags") \
		.must_contain("current_tag", "current") \
		.verify()

	assert_json(json_second).describe("current scope stays in effect for later events on the same thread") \
		.at("/tags") \
		.must_contain("current_tag", "current") \
		.verify()


func test_with_scope_on_thread_excludes_main_scope() -> void:
	var main_in_scope := Semaphore.new()
	var worker_captured := Semaphore.new()

	var thread := Thread.new()
	thread.start(func() -> void:
		main_in_scope.wait()
		SentrySDK.with_scope(func(scope: SentryScope) -> void:
			scope.set_tag("worker_tag", "worker")
			SentrySDK.capture_event(SentrySDK.create_event())
			)
		worker_captured.post()
		)

	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_tag("main_tag", "main")
		main_in_scope.post()
		worker_captured.wait()
		)
	thread.wait_to_finish()

	var json_worker: String = await wait_for_captured_event_json()

	assert_json(json_worker).describe("worker event carries the worker scope, not the main thread's") \
		.at("/tags") \
		.must_contain("worker_tag", "worker") \
		.must_not_contain("main_tag") \
		.verify()


func test_with_scope_on_main_excludes_thread_scope() -> void:
	var worker_in_scope := Semaphore.new()
	var main_captured := Semaphore.new()

	var thread := Thread.new()
	thread.start(func() -> void:
		SentrySDK.with_scope(func(scope: SentryScope) -> void:
			scope.set_tag("worker_tag", "worker")
			worker_in_scope.post()
			main_captured.wait()
			)
		)

	worker_in_scope.wait()
	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_tag("main_tag", "main")
		SentrySDK.capture_event(SentrySDK.create_event())
		)
	main_captured.post()
	thread.wait_to_finish()

	var json_main: String = await wait_for_captured_event_json()

	assert_json(json_main).describe("main event carries the main scope, not the worker thread's") \
		.at("/tags") \
		.must_contain("main_tag", "main") \
		.must_not_contain("worker_tag") \
		.verify()


func test_with_scope_on_thread_inherits_global() -> void:
	SentrySDK.set_tag("global_for_worker", "global")

	var thread := Thread.new()
	thread.start(func() -> void:
		SentrySDK.with_scope(func(scope: SentryScope) -> void:
			scope.set_tag("worker_tag", "worker")
			SentrySDK.capture_event(SentrySDK.create_event())
			)
		)
	thread.wait_to_finish()
	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("worker thread scope layers over the global scope") \
		.at("/tags") \
		.must_contain("global_for_worker", "global") \
		.must_contain("worker_tag", "worker") \
		.verify()


## Resumes the coroutine parked by test_with_scope_does_not_leak_while_parked().
signal resume_parked_coroutine


func test_with_scope_does_not_leak_while_parked() -> void:

	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_tag("parked_tag", "in_parked_scope")
		await resume_parked_coroutine
		)

	var json_while_parked: String = await capture_event_and_get_json(SentrySDK.create_event())

	resume_parked_coroutine.emit()

	assert_json(json_while_parked).describe("event captured while a with_scope() coroutine is parked does not carry the parked scope's tag") \
		.at("/tags") \
		.must_not_contain("parked_tag") \
		.verify()
