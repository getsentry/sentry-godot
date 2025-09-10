class_name ProjectMainLoop
extends SceneTree
## Example of initializing and configuring Sentry from code.
##
## The earliest place to initialize Sentry in script is in the MainLoop._initialize().
## Tip: You can assign "ProjectMainLoop" as your main loop class in the project settings
##      under `application/run/main_loop_type`.


func _initialize() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		print("INFO: [ProjectMainLoop] Initializing SDK from GDScript")

		options.debug = true
		options.release = "sentry-godot-demo@" + ProjectSettings.get_setting("application/config/version")
		options.environment = "demo"

		# Set up event callbacks
		options.before_send = _on_before_send_to_sentry
	)

	# Post-initialize
	# SentrySDK.add_attachment(...)
	# ...


## before_send example
func _on_before_send_to_sentry(ev: SentryEvent) -> SentryEvent:
	print("INFO: [ProjectMainLoop] Processing event: ", ev.id)
	if ev.message.contains("Bruno"):
		print("INFO: [ProjectMainLoop] Removing sensitive information from the event")
		ev.message = ev.message.replace("Bruno", "REDACTED")
	elif ev.message == "junk":
		print("INFO: [ProjectMainLoop] Discarding event with message 'junk'")
		return null
	return ev
