class_name CLICommands
extends Node
## Contains command functions that can be executed via CLI.
##
## A command must return a POSIX-compliant integer exit code, where 0 indicates success.
## Exit codes are typically in the range of 0–125.[br][br]
##
## Usage:[br]
##     godot --headless --path ./project -- COMMAND [ARGS...]
##
## [br][br]
## Use "godot --headless --path ./project -- help" to list available commands.


var exit_code: int

var _parser := CLIParser.new()


func _ready() -> void:
	_register_commands()


## Checks and executes CLI commands if found.
## Returns true if a command was executed (caller should handle app exit).
func check_and_execute_cli() -> bool:
	var executed: bool = await _parser.check_and_execute_cli()
	exit_code = _parser.exit_code
	return executed


## Registers all available commands with the parser.
func _register_commands() -> void:
	_parser.add_command("help", _cmd_help, "Show available commands")
	_parser.add_command("crash-capture", _cmd_crash_capture, "Generate a controlled crash for testing")
	_parser.add_command("crash-send", _cmd_crash_send, "Process and send crash report from previous session")
	_parser.add_command("message-capture", _cmd_message_capture, "Capture a test message to Sentry")
	_parser.add_command("runtime-error-capture", _cmd_runtime_error_capture, "Capture Godot runtime error")
	_parser.add_command("attachment-capture", _cmd_attachment_capture, "Capture a message with custom attachments")
	_parser.add_command("log-capture", _cmd_log_capture, "Capture a structured log to Sentry")
	_parser.add_command("run-tests", _cmd_run_tests, "Run unit tests")


## Shows available commands and their arguments.
func _cmd_help() -> int:
	print(_parser.generate_help())
	return 0


## Generates a controlled crash for testing.
func _cmd_crash_capture() -> int:
	await _init_sentry()
	_add_integration_test_context("crash-capture")

	print("Triggering controlled crash...")

	# NOTE: Borrowing UUID generation from SentryUser class.
	var uuid_gen := SentryUser.new()
	uuid_gen.generate_new_id()
	SentrySDK.set_tag("test.crash_id", uuid_gen.id)
	print("EVENT_CAPTURED: ", uuid_gen.id)

	_print_test_result("crash-capture", true, "Pre-crash setup complete")
	SentrySDK.add_breadcrumb(SentryBreadcrumb.create("About to trigger controlled crash"))

	# Wait for scope to sync with NDK layer on Android
	# NOTE: On Android, NDK scope sync seems to happen with delay.
	await get_tree().create_timer(0.5).timeout

	# Use the same crash method as the demo
	SentrySDK._demo_helper_crash_app()
	return 0


## Initializes Sentry to process and send any crash report from previous session.
func _cmd_crash_send() -> int:
	print("Initializing Sentry so it can send crash report...")
	await _init_sentry()
	_add_integration_test_context("crash-send")
	# Wait 10 iterations
	for i in range(10):
		await get_tree().process_frame
	return 0


## Captures a test message to Sentry.
func _cmd_message_capture(p_message: String = "Integration test message", p_level: String = "info") -> int:
	await _init_sentry()
	_add_integration_test_context("message-capture")

	print("Capturing message: '%s' with level: %s" % [p_message, p_level])

	var level: SentrySDK.Level
	match p_level.to_lower():
		"debug": level = SentrySDK.LEVEL_DEBUG
		"info": level = SentrySDK.LEVEL_INFO
		"warning", "warn": level = SentrySDK.LEVEL_WARNING
		"error": level = SentrySDK.LEVEL_ERROR
		"fatal": level = SentrySDK.LEVEL_FATAL
		_:
			printerr("Warning: Unknown level '%s', using INFO" % p_level)
			level = SentrySDK.LEVEL_INFO

	var event_id := SentrySDK.capture_message(p_message, level)
	print("EVENT_CAPTURED: ", event_id)
	_print_test_result("message-capture", true, "Test complete")
	return 0


func _cmd_runtime_error_capture() -> int:
	await _init_sentry()
	_add_integration_test_context("runtime-error-capture")

	print("Triggering runtime error...")
	var stack: Array = _trigger_runtime_error()
	print("EVENT_CAPTURED: ", SentrySDK.get_last_event_id())

	# Print stack frames in expected order and format
	for frame: Dictionary in stack:
		print("FRAME: {filename} | {function} | {line}".format({
			filename=frame.source,
			function=frame.function,
			line=frame.line
		}))

	return 0


## Captures a message with custom attachments to Sentry.
func _cmd_attachment_capture() -> int:
	# Write fixture files to user:// so they exist on disk on all platforms (res:// is packed in exports).
	_write_text_file("user://config_attachment.txt", "Config file attachment for integration testing.\n")
	_write_text_file("user://runtime_attachment.txt", "Runtime file attachment for integration testing.\n")

	# Add attachments during init to test that they are retained after SDK initialization.
	await _init_sentry(func(options: SentryOptions) -> void:
		# Enable default attachments (screenshot excluded — not available in headless/CLI mode)
		options.attach_log = true
		options.attach_scene_tree = true
		# File attachment added in config callback
		SentrySDK.add_attachment(SentryAttachment.create_with_path("user://config_attachment.txt"))
		# Bytes attachment added in config callback
		var config_bytes := SentryAttachment.create_with_bytes(
			"Config bytes".to_utf8_buffer(), "config_bytes.txt")
		config_bytes.content_type = "text/plain"
		SentrySDK.add_attachment(config_bytes)
	)
	_add_integration_test_context("attachment-capture")

	# File attachment added after init
	SentrySDK.add_attachment(SentryAttachment.create_with_path("user://runtime_attachment.txt"))
	# Bytes attachment added after init
	var runtime_bytes := SentryAttachment.create_with_bytes(
		"Runtime bytes".to_utf8_buffer(), "runtime_bytes.txt")
	runtime_bytes.content_type = "text/plain"
	SentrySDK.add_attachment(runtime_bytes)

	var event_id := SentrySDK.capture_message("Attachment test message")
	print("EVENT_CAPTURED: ", event_id)
	_print_test_result("attachment-capture", true, "Test complete")
	return 0


## Captures a structured log to Sentry.
func _cmd_log_capture() -> int:
	await _init_sentry(func(options: SentryOptions) -> void:
		options.enable_logs = true
		options.before_send_log = _before_send_log
	)
	_add_integration_test_context("log-capture")

	# Generate unique test ID for correlation
	# NOTE: Borrowing UUID generation from SentryUser class.
	var uuid_gen := SentryUser.new()
	uuid_gen.generate_new_id()
	var test_id := uuid_gen.id

	# Set global attributes (merged into all logs)
	SentrySDK.set_attribute("global_attribute", "global_value")
	SentrySDK.set_attribute("deleted_global_attribute", "should_not_appear")
	SentrySDK.remove_attribute("deleted_global_attribute")

	# Send structured log with attributes
	SentrySDK.logger.warn("Integration test structured log", [], {
		"test_id": test_id,
		"deleted_log_attribute": "original_value",
	})

	print("LOG_TRIGGERED: ", test_id)

	# Flush pending data before exit
	SentrySDK.close()
	await get_tree().create_timer(1.0).timeout

	_print_test_result("log-capture", true, "Test complete")
	return 0


func _before_send_log(entry: SentryLog) -> SentryLog:
	entry.set_attribute("handler_added", "added_value")
	entry.remove_attribute("deleted_log_attribute")
	return entry


func _cmd_run_tests(tests: String = "res://test/suites/") -> int:
	if FileAccess.file_exists("res://test/util/test_runner.gd"):
		print(">>> Initializing testing")
		await get_tree().process_frame

		var included_paths: PackedStringArray = tests.split(";", false)
		if included_paths.is_empty():
			printerr("No test path provided.")
			return 1
		print(" -- Tests included: ", included_paths)

		# Add test runner node.
		print(" -- Adding test runner...")
		var test_runner: Node = load("res://test/util/test_runner.gd").new()
		get_tree().root.add_child(test_runner)
		for path in included_paths:
			test_runner.include_tests(path)

		# Wait for completion.
		await test_runner.finished
		print(">>> Test run complete with code: ", str(test_runner.result_code))

		return test_runner.result_code
	else:
		printerr("Error: Test runner not found")
		return 1


## Initializes Sentry for integration testing.
func _init_sentry(p_extra_config: Callable = Callable()) -> void:
	print("Initializing Sentry...")

	SentrySDK.init(func(options: SentryOptions) -> void:
		options.debug = false
		options.diagnostic_level = SentrySDK.LEVEL_ERROR
		options.release = "test-app@1.0.0"
		options.environment = "integration-test"
		options.dist = "test-dist"
		if p_extra_config.is_valid():
			p_extra_config.call(options)
	)

	# Wait for Sentry to initialize
	await get_tree().create_timer(0.5).timeout


## Add additional context for integration tests.
func _add_integration_test_context(p_command: String) -> void:
	SentrySDK.add_breadcrumb(SentryBreadcrumb.create("Integration test started"))

	var user := SentryUser.new()
	user.id = "12345"
	user.username = "TestUser"
	user.email = "user-mail@test.abc"
	SentrySDK.set_user(user)

	SentrySDK.set_tag("test.suite", "integration")
	SentrySDK.set_tag("test.type", p_command)

	SentrySDK.add_breadcrumb(SentryBreadcrumb.create("Context configuration finished"))


func _trigger_runtime_error() -> Array:
	# NOTE: There should be 5 lines of code before and after push_error(), so we can validate source context.
	var stack: Array = get_stack()
	push_error("Runtime error")
	stack[0].line += 1 # Adjust to actual error line
	stack.reverse() # Godot stacks are in reverse order
	return stack


func _write_text_file(p_path: String, p_content: String) -> void:
	var file := FileAccess.open(p_path, FileAccess.WRITE)
	file.store_string(p_content)
	file.close()


func _print_test_result(test_name: String, success: bool, message: String) -> void:
	print("TEST_RESULT: {\"test\":\"%s\",\"success\":%s,\"message\":\"%s\"}" % [
		test_name,
		success,
		message
	])
