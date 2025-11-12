class_name CLICommands
extends Node
## Contains command functions that can be executed via CLI. [br]
##
## Usage: [br]
##     godot --headless --path ./project -- COMMAND [ARGS...]
##
## Use "godot --headless --path ./project -- help" to list available commands.


var exit_code: int

var _parser := CLIParser.new()


func _ready() -> void:
	_register_available_commands()


## Checks and executes CLI commands if found.
## Returns true if a command was executed (caller should handle app exit).
func check_and_execute_cli() -> bool:
	var executed: bool = await _parser.check_and_execute_cli()
	exit_code = _parser.exit_code
	return executed


## Registers all available commands with the parser.
func _register_available_commands() -> void:
	_parser.add_command("help", _cmd_help, "Show available commands")
	_parser.add_command("message-capture", _cmd_message_capture, "Capture a test message to Sentry")
	_parser.add_command("crash-capture", _cmd_crash_capture, "Generate a controlled crash for testing")
	_parser.add_command("run-tests", _cmd_run_tests, "Run unit tests")


## Initializes Sentry.
func _init_sentry() -> void:
	print("Initializing Sentry...")

	SentrySDK.init(func(options: SentryOptions) -> void:
		options.debug = true
		options.release = "sentry-godot-cli-test@1.0.0"
		options.environment = "cli-testing"
	)

	# Wait for Sentry to initialize
	await get_tree().create_timer(0.3).timeout


## Shows available commands and their arguments.
func _cmd_help() -> int:
	print(_parser.generate_help())
	return 0


## Captures a test message to Sentry.
func _cmd_message_capture(p_message: String = "Test message from CLI", p_level: String = "info") -> int:
	print("Capturing message: '%s' with level: %s" % [p_message, p_level])

	_init_sentry()

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
	return 0


## Generates a controlled crash for testing.
func _cmd_crash_capture() -> int:
	_init_sentry()

	print("Generating crash...")
	await get_tree().create_timer(0.5).timeout

	# Use the same crash method as the demo
	SentrySDK._demo_helper_crash_app()
	return 0


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
