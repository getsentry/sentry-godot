extends SentryConfiguration
## Example Sentry configuration script.
##
## Tip: You can assign configuration script in the project settings.


## Configure Sentry SDK options
func _configure(options: SentryOptions) -> void:
	print("INFO: [example_configuration.gd] Configuring SDK options via GDScript")

	options.debug = true
	options.release = "sentry-godot-demo@" + ProjectSettings.get_setting("application/config/version")
	options.environment = "demo"

	# Set up event callbacks
	options.before_send = _before_send

	# Unit testing hooks (if you're exploring the demo project, just ignore the following line).
	load("res://testing_configuration.gd").configure_options(options)


## before_send callback example
func _before_send(ev: SentryEvent) -> SentryEvent:
	print("INFO: [example_configuration.gd] Processing event: ", ev.id)
	if ev.message.contains("Bruno"):
		print("INFO: [example_configuration.gd] Removing sensitive information from the event")
		ev.message = ev.message.replace("Bruno", "REDACTED")
	elif ev.message == "junk":
		print("INFO: [example_configuration.gd] Discarding event with message 'junk'")
		return null
	return ev
