extends SentryTestSuite
## Test SentryGodotLogger error capture with detailed JSON validation.
## NOTE: Additional logger tests can be found in test/isolated/ with filenames beginning with "test_logger_".


func test_gdscript_error_event_structure() -> void:
	push_error("Test error message")

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("Logger event has basic required attributes") \
		.at("/") \
		.must_contain("event_id") \
		.must_contain("timestamp") \
		.must_contain("level", "error") \
		.verify()

	assert_json(json).describe("Logger event has logger attribute") \
		.at("/") \
		.must_contain("logger", "SentryGodotLogger") \
		.verify()

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


func test_past_errors_appear_as_breadcrumbs() -> void:
	push_error("first error")
	await wait_for_captured_event_json()

	push_error("second error")
	var second_event: String = await wait_for_captured_event_json()

	assert_int(captured_events.size()).is_equal(2).override_failure_message("expected two events")

	assert_json(second_event).describe("First error captured should be the last breadcrumb") \
		.at("/breadcrumbs/") \
		.with_objects() \
		.containing("message", "first error") \
		.must_contain("level", "error") \
		.must_contain("category", "error") \
		.exactly(1)

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
		.containing("filename", get_script().resource_path) \
		.at_least(1)


func test_gdscript_error_stacktrace_deep() -> void:
	var backtraces: Array[ScriptBacktrace] = Engine.capture_script_backtraces(true)
	push_error("Stacktrace frames test")

	# Find GDScript backtrace
	var backtrace: ScriptBacktrace
	for bt in backtraces:
		if bt.get_language_name() == "GDScript":
			backtrace = bt
			break

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
