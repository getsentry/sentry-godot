extends SentryTestSuite
## Verify Godot errors are properly adding breadcrumbs.


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.logger_messages_as_breadcrumbs = true
	)


func test_logger_warnings_and_prints_create_breadcrumbs() -> void:
	# We expect print() and push_warning() to become breadcrumbs and not create events.
	print("Debug message")
	push_warning("Warning message")
	push_error("Final error message")

	var json: String = await wait_for_captured_event_json()
	assert_int(captured_events.size()).is_equal(1).override_failure_message("expected a single event")

	assert_json(json).describe("print() should appear as the pre-last breadcrumb") \
		.at("/breadcrumbs/-2") \
		.must_contain("message", "Debug message") \
		.must_contain("level", "info") \
		.must_contain("category", "log") \
		.verify()

	assert_json(json).describe("Warning should appear as the last breadcrumb") \
		.at("/breadcrumbs/-1") \
		.must_contain("message", "Warning message") \
		.must_contain("level", "warning") \
		.must_contain("category", "error") \
		.verify()
