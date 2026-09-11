extends SentryTestSuite
## Verify Godot errors are properly adding breadcrumbs.


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.godot_logger.breadcrumb_mask |= SentryOptions.MASK_MESSAGE
	)


func test_logger_warnings_and_prints_create_breadcrumbs() -> void:
	# We expect print() and push_warning() to become breadcrumbs and not create events.
	print("Debug message")
	push_warning("Warning message")
	push_error("Final error message")

	var json: String = await wait_for_captured_event_json()
	assert_int(captured_events.size()).is_equal(1).override_failure_message("expected a single event")

	var breadcrumbs: Array = JSON.parse_string(json).get("breadcrumbs", [])
	var logger_breadcrumbs := breadcrumbs.filter(func(crumb: Dictionary) -> bool:
		return crumb.get("message") in ["Debug message", "Warning message"]
	)
	assert_array(logger_breadcrumbs).has_size(2)

	assert_json(logger_breadcrumbs).describe("print() should appear before the warning breadcrumb") \
		.at("/0") \
		.must_contain("message", "Debug message") \
		.must_contain("level", "info") \
		.must_contain("category", "log") \
		.verify()

	assert_json(logger_breadcrumbs).describe("Warning should appear after the print breadcrumb") \
		.at("/1") \
		.must_contain("message", "Warning message") \
		.must_contain("level", "warning") \
		.must_contain("category", "error") \
		.verify()
