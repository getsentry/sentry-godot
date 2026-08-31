extends SentryTestSuite
## Verifies the `before_send_feedback` callback and what user feedback carries.


signal feedback_captured

var captured_feedback: Array[String]

var before_send_feedback: Callable # (event: SentryEvent) -> SentryEvent

## Position of the next feedback to be returned by wait_for_captured_feedback_json().
var _next_captured_feedback := 0


# TODO: drop the skip when Cocoa gains a feedback hook.
func before(_do_skip = OS.get_name() in ["macOS", "iOS"],
		_skip_reason = "before_send_feedback is not supported on this platform yet.") -> void:
	super()


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.before_send_feedback = func(event: SentryEvent) -> SentryEvent:
			return before_send_feedback.call(event)
	)


func before_test() -> void:
	super()
	captured_feedback.clear()
	_next_captured_feedback = 0
	before_send_feedback = _default_before_send_feedback


func _default_before_send_feedback(event: SentryEvent) -> SentryEvent:
	captured_feedback.append(event.to_json())
	feedback_captured.emit()
	return null


## Expect feedback to be processed and return its serialized JSON content.
## Consecutive calls hand out captured feedback in capture order.
func wait_for_captured_feedback_json() -> String:
	if _next_captured_feedback >= captured_feedback.size():
		await await_signal_on(self, "feedback_captured")
	if _next_captured_feedback >= captured_feedback.size():
		return ""
	var json: String = captured_feedback[_next_captured_feedback]
	_next_captured_feedback += 1
	return json


func test_submitted_fields_reach_the_callback() -> void:
	var feedback := SentryFeedback.new()
	feedback.message = "The boss fight is unbeatable"
	feedback.name = "Bob"
	feedback.contact_email = "bob@example.com"
	feedback.associated_event_id = "082ce03eface41dd94b8c6b005382d5e"

	SentrySDK.capture_feedback(feedback)
	var json: String = await wait_for_captured_feedback_json()

	assert_json(json).describe("feedback context carries the submitted fields") \
		.at("/contexts/feedback") \
		.is_object() \
		.must_contain("message", "The boss fight is unbeatable") \
		.must_contain("name", "Bob") \
		.must_contain("contact_email", "bob@example.com") \
		.must_contain("associated_event_id", "082ce03eface41dd94b8c6b005382d5e") \
		.verify()


func test_callback_can_modify_the_event() -> void:
	before_send_feedback = func(event: SentryEvent) -> SentryEvent:
		event.set_tag("feedback_source", "test_suite")
		event.level = SentrySDK.LEVEL_WARNING
		return _default_before_send_feedback(event)

	var feedback := SentryFeedback.new()
	feedback.message = "Modified feedback"
	SentrySDK.capture_feedback(feedback)
	var json: String = await wait_for_captured_feedback_json()

	assert_json(json).describe("tag set in the callback reaches the event") \
		.at("/tags") \
		.must_contain("feedback_source", "test_suite") \
		.verify()

	assert_json(json).describe("level set in the callback reaches the event") \
		.at("/") \
		.must_contain("level", "warning") \
		.verify()


func test_callback_reads_the_submitted_fields() -> void:
	var seen := {}
	before_send_feedback = func(event: SentryEvent) -> SentryEvent:
		var submitted := event.get_feedback()
		seen["message"] = submitted.message
		seen["name"] = submitted.name
		seen["contact_email"] = submitted.contact_email
		seen["associated_event_id"] = submitted.associated_event_id
		return _default_before_send_feedback(event)

	var feedback := SentryFeedback.new()
	feedback.message = "The elevator eats my keys 🔑"
	feedback.name = "Alice"
	feedback.contact_email = "alice@example.com"
	feedback.associated_event_id = "3c9f2b1ae5d4487fa1b0d5c6e7f80912"

	SentrySDK.capture_feedback(feedback)
	await wait_for_captured_feedback_json()

	assert_str(seen["message"]).is_equal("The elevator eats my keys 🔑")
	assert_str(seen["name"]).is_equal("Alice")
	assert_str(seen["contact_email"]).is_equal("alice@example.com")
	assert_str(seen["associated_event_id"]).is_equal("3c9f2b1ae5d4487fa1b0d5c6e7f80912")


func test_callback_can_scrub_the_feedback() -> void:
	before_send_feedback = func(event: SentryEvent) -> SentryEvent:
		var submitted := event.get_feedback()
		submitted.message = "[redacted]"
		submitted.name = "Anonymous"
		submitted.contact_email = ""
		return _default_before_send_feedback(event)

	var feedback := SentryFeedback.new()
	feedback.message = "My password is hunter2"
	feedback.name = "Bob"
	feedback.contact_email = "bob@example.com"

	SentrySDK.capture_feedback(feedback)
	var json: String = await wait_for_captured_feedback_json()

	assert_json(json).describe("fields rewritten in the callback reach the event") \
		.at("/contexts/feedback") \
		.is_object() \
		.must_contain("message", "[redacted]") \
		.must_contain("name", "Anonymous") \
		.must_not_contain("contact_email") \
		.verify()


func test_feedback_does_not_reach_before_send() -> void:
	var feedback := SentryFeedback.new()
	feedback.message = "Feedback stays out of before_send"
	SentrySDK.capture_feedback(feedback)
	assert_str(await wait_for_captured_feedback_json()).is_not_empty()

	await get_tree().process_frame  # ensure before_send doesn't run right after

	assert_int(captured_events.size()).override_failure_message(
		"Feedback should not have reached before_send").is_equal(0)


func test_enrichment_reaches_feedback() -> void:
	var feedback := SentryFeedback.new()
	feedback.message = "Enriched feedback"
	SentrySDK.capture_feedback(feedback)

	assert_json(await wait_for_captured_feedback_json()) \
		.describe("processor-injected contexts reach the feedback event") \
		.at("/contexts/godot_performance") \
		.is_object() \
		.verify()


## Feedback captured inside with_scope() carries the scoped data, and later captures do not.
func test_scoped_feedback_capture() -> void:
	var scoped_user := SentryUser.new()
	scoped_user.id = "player_scope"

	SentrySDK.with_scope(func(scope: SentryScope) -> void:
		scope.set_tag("scoped", "in_scope")
		scope.set_context("scene", {"name": "Dungeon", "depth": 3})
		scope.set_user(scoped_user)
		var feedback := SentryFeedback.new()
		feedback.message = "Scoped feedback"
		SentrySDK.capture_feedback(feedback)
		)
	var json_in_scope: String = await wait_for_captured_feedback_json()

	var after := SentryFeedback.new()
	after.message = "Feedback after the scope"
	SentrySDK.capture_feedback(after)
	var json_after: String = await wait_for_captured_feedback_json()

	assert_json(json_in_scope).describe("scoped tag reaches the in-scope feedback") \
		.at("/tags") \
		.must_contain("scoped", "in_scope") \
		.verify()

	assert_json(json_in_scope).describe("scoped context reaches the in-scope feedback") \
		.at("/contexts/scene") \
		.is_object() \
		.must_contain("name", "Dungeon") \
		.must_contain("depth", 3) \
		.verify()

	assert_json(json_in_scope).describe("scoped user reaches the in-scope feedback") \
		.at("/user") \
		.must_contain("id", "player_scope") \
		.verify()

	assert_json(json_after).describe("scoped data does not leak past with_scope") \
		.at("/") \
		.must_not_contain("/tags/scoped") \
		.must_not_contain("/contexts/scene") \
		.verify()
