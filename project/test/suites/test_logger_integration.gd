extends SentryTestSuite
## Test SentryLogger error capture with detailed JSON validation.
## NOTE: Logger limits are tested separately in isolated tests.


func test_gdscript_error_event_structure() -> void:
	push_error("Test error message")

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("Logger event has basic required attributes") \
		.at("/") \
		.must_contain("event_id") \
		.must_contain("timestamp") \
		.must_contain("level", "error") \
		.verify()

	# NOTE: Logger events from push_error() do not include a "logger" field
	# TODO: Set logger attribute in SentryLogger

	assert_json(json).describe("Logger error event contains exception data") \
		.at("/exception") \
		.is_object() \
		.must_contain("values") \
		.verify()

	assert_json(json).describe("Exception values array contains a single GDScript error") \
		.at("/exception/values") \
		.is_array() \
		.with_objects() \
		.must_contain("type", "ERROR") \
		.must_contain("value", "Test error message") \
		.exactly(1)


func test_logger_warnings_and_prints_create_breadcrumbs() -> void:
	# NOTE: Assuming that logger_messages_as_breadcrumbs is enabled
	#		and warning events are disabled (the default).
	# TODO: move to isolation ???

	# We expect print() and push_warning() to become breadcrumbs and not create events.
	print("Debug message")
	push_warning("Warning message")
	push_error("Final error message")

	var json: String = await wait_for_captured_event_json()
	assert_int(captured_events.size()).is_equal(1).override_failure_message("expected a single event")

	# NOTE: messages_as_breadcrumbs is disabled in SentryTestSuite – so we shouldn't test this here.
	#assert_json(json).describe("print() should appear as the pre-last breadcrumb") \
		#.at("/breadcrumbs/-2") \
		#.containing("message", "Debug message\n") \
		#.must_contain("level", "info") \
		#.must_contain("category", "log") \
		#.exactly(1)

	assert_json(json).describe("Warning should appear as the last breadcrumb") \
		.at("/breadcrumbs/-1") \
		.containing("message", "Warning message") \
		.must_contain("level", "warning") \
		.must_contain("category", "error") \
		.verify()


func test_past_errors_appear_as_breadcrumbs() -> void:
	push_error("first error")
	await wait_for_captured_event_json()

	push_error("second error")
	var second_event: String = await wait_for_captured_event_json()

	assert_int(captured_events.size()).is_equal(2).override_failure_message("expected two events")

	assert_json(second_event).describe("First error captured should be the last breadcrumb") \
		.at("/breadcrumbs/-1") \
		.containing("message", "first error") \
		.must_contain("level", "error") \
		.must_contain("category", "error") \
		.verify()

	assert_json(second_event).describe("Second (aka current) error should NOT be in breadcrumbs") \
		.at("/breadcrumbs/") \
		.with_objects() \
		.containing("message", "second error") \
		.exactly(0)


func test_gdscript_error_stacktrace_basic() -> void:
	# Push error and capture line number
	var expected_line: int = get_stack()[0].line + 1
	push_error("Stacktrace basic test")

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("Event has threads with values") \
		.at("/threads") \
		.is_object() \
		.must_contain("values") \
		.verify()

	assert_json(json).describe("Thread values contains stacktrace") \
		.at("/threads/values") \
		.is_array() \
		.with_objects() \
		.must_contain("stacktrace") \
		.at_least(1)

	assert_json(json).describe("GDScript frames appear in stacktrace with filename and lineno attributes") \
		.at("/threads/values/0/stacktrace/frames") \
		.is_array() \
		.with_objects() \
		.containing("platform", "gdscript") \
		.must_contain("filename") \
		.must_contain("lineno") \
		.at_least(1)

	assert_json(json).describe("GDScript frames contain a frame with expected filename and lineno") \
		.at("/threads/values/0/stacktrace/frames") \
		.is_array() \
		.with_objects() \
		.containing("platform", "gdscript") \
		.containing("lineno", expected_line) \
		.must_contain("filename", get_script().resource_path) \
		.at_least(1)


func test_gdscript_error_stacktrace_deep() -> void:
	var backtrace: ScriptBacktrace = Engine.capture_script_backtraces(true)[0]
	push_error("Stacktrace frames test")

	# Collect info from backtrace
	var frames: Array[Dictionary]
	for i in backtrace.get_frame_count():
		var frame := {}
		# Adjusting first frame lineno to actual push_error() line
		frame.lineno = backtrace.get_frame_line(i) + (1 if i == 0 else 0)
		frame.filename = backtrace.get_frame_file(i)
		frame.function = backtrace.get_frame_function(i)
		frames.push_front(frame) # reverse frame order

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("Event contains some GDScript frames") \
		.at("/threads/values/0/stacktrace/frames") \
		.is_array() \
		.with_objects() \
		.containing("platform", "gdscript") \
		.at_least(1)

	for i in frames.size():
		var frame: Dictionary = frames[i]
		assert_json(json).describe("GDScript frames contain proper frame #" + str(i)) \
			.at("/threads/values/0/stacktrace/frames/%d" % i) \
			.is_object() \
			.must_contain("lineno", frame.lineno) \
			.must_contain("filename", frame.filename) \
			.must_contain("function", frame.function) \
			.must_contain("platform", "gdscript") \
			.must_contain("in_app", true) \
			.must_contain("context_line") \
			.must_contain("pre_context") \
			.must_contain("post_context") \
			.verify()


func test_local_variables_capture() -> void:
	# Set up local variables for capture
	@warning_ignore("unused_variable")
	var test_string: String = "captured_value"
	@warning_ignore("unused_variable")
	var test_utf8: String = "こんにちは 🌍"
	@warning_ignore("unused_variable")
	var test_number: int = 42
	@warning_ignore("unused_variable")
	var test_array: Array = ["item1", "item2"]

	push_error("Variable capture test")

	# TODO: Check if disabling variables removes them also from the frames in isolated testing.

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("Stacktrace contains some frames") \
		.at("/threads/values/0/stacktrace/frames") \
		.is_array() \
		.is_not_empty() \
		.verify()

	assert_json(json).describe("For this file, a frame exists that contains proper variable values") \
		.at("/threads/values/0/stacktrace/frames/") \
		.is_array() \
		.with_objects() \
		.containing("filename", get_script().resource_path) \
		.must_contain("vars/test_string", "captured_value") \
		.must_contain("vars/test_utf8", "こんにちは 🌍") \
		.must_contain("vars/test_number", 42) \
		.must_contain("vars/test_array", ["item1", "item2"]) \
		.exactly(1)


func test_exception_value_with_utf8() -> void:
	push_error("Error with UTF-8: 世界 🌍")
	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("UTF-8 characters are preserved in error messages") \
		.at("/exception/values/0/value") \
		.must_be("Error with UTF-8: 世界 🌍") \
		.verify()
