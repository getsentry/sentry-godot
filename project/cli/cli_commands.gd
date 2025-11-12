class_name CLICommands
extends Node
## CLI commands implementation for testing Sentry SDK
##
## Contains the actual command functions that can be executed via CLI. [br]
##
## Usage: [br]
##     godot --headless --path ./project -- COMMAND [ARGS...]


var _parser: CLIParser


func _ready() -> void:
	_parser = load("res://cli/cli_parser.gd").new()
	_register_available_commands()


## Returns the exit code of the last executed command.
func get_exit_code() -> int:
	return _parser.exit_code


## Checks and executes CLI commands if found.
## Returns true if a command was executed (caller should handle app exit).
func check_and_execute_cli() -> bool:
	if await _parser.check_and_execute_cli():
		return true
	return false


## Registers all available commands with the parser.
func _register_available_commands() -> void:
	_parser.add_command("help", _cmd_help, "Show available commands")
	_parser.add_command("message-capture", _cmd_message_capture, "Capture a test message to Sentry")
	_parser.add_command("crash-capture", _cmd_crash_capture, "Generate a controlled crash for testing")


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


## Captures a test message to Sentry.
func _cmd_message_capture(p_message: String = "Test message from CLI", p_level: String = "info") -> void:
	print("Capturing message: '%s' with level: %s" % [p_message, p_level])

	_init_sentry()

	var sentry_level: int
	match p_level.to_lower():
		"debug": sentry_level = SentrySDK.LEVEL_DEBUG
		"info": sentry_level = SentrySDK.LEVEL_INFO
		"warning", "warn": sentry_level = SentrySDK.LEVEL_WARNING
		"error": sentry_level = SentrySDK.LEVEL_ERROR
		"fatal": sentry_level = SentrySDK.LEVEL_FATAL
		_:
			printerr("Warning: Unknown level '%s', using INFO" % p_level)
			sentry_level = SentrySDK.LEVEL_INFO

	var event_id := SentrySDK.capture_message(p_message, sentry_level)
	printerr("Message captured with event ID: ", event_id)


## Generates a controlled crash for testing.
func _cmd_crash_capture() -> void:
	_init_sentry()

	print("Generating crash...")
	await get_tree().create_timer(0.5).timeout

	# Use the same crash method as the demo
	SentrySDK._demo_helper_crash_app()


## Shows available commands and their arguments.
func _cmd_help() -> void:
	print(_parser.generate_help())
