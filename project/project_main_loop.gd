class_name ProjectMainLoop
extends SceneTree
## Example of initializing and configuring Sentry from code.
##
## The earliest place to initialize Sentry in script is in the MainLoop._initialize().
## Tip: You can assign "ProjectMainLoop" as your main loop class in the project settings
##      under `application/run/main_loop_type`.


signal before_send_log(log_entry)


func _initialize() -> void:
	if await _run_tests_if_needed():
		return

	SentrySDK.init(func(options: SentryOptions) -> void:
		print("INFO: [ProjectMainLoop] Initializing SDK from GDScript")

		options.debug = true
		options.release = "sentry-godot-demo@" + ProjectSettings.get_setting("application/config/version")
		options.environment = "demo"

		# Set up event callbacks
		options.before_send = _on_before_send_to_sentry
		options.experimental.before_send_log = _on_before_send_log_to_sentry
	)

	# Post-initialize
	# SentrySDK.add_attachment(...)
	# ...


## before_send example
func _on_before_send_to_sentry(ev: SentryEvent) -> SentryEvent:
	print("INFO: [ProjectMainLoop] Processing event: ", ev.id)
	var error_message: String = ev.get_exception_value(0)
	if error_message.contains("Bruno"):
		print("INFO: [ProjectMainLoop] Removing sensitive information from the event")
		var redacted_message := error_message.replace("Bruno", "REDACTED")
		ev.set_exception_value(0, redacted_message)
	elif error_message == "junk":
		print("INFO: [ProjectMainLoop] Discarding event with error message 'junk'")
		return null
	return ev


## before_send_log
func _on_before_send_log_to_sentry(entry: SentryLog) -> SentryLog:
	before_send_log.emit(entry)
	return entry


func _is_running_tests_from_editor() -> bool:
	return "res://addons/gdUnit4/src/core/runners/GdUnitTestRunner.tscn" in OS.get_cmdline_args()


## Returns true if tests being executed. [i]Async.[/i]
func _run_tests_if_needed() -> bool:
	if _is_running_tests_from_editor():
		return true
	if FileAccess.file_exists("res://test/util/test_run.gd"):
		var test_run = load("res://test/util/test_run.gd").new()
		if test_run.should_run():
			await test_run.execute()
			return true
	return false
