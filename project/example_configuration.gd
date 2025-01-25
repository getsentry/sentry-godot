extends SentryConfiguration
## Example Sentry configuration script.
##
## Tip: You can assign configuration script in the project settings.


## Configure Sentry SDK options
func _configure(options: SentryOptions) -> void:
	print("[example_configuration.gd] Configuring SDK options via GDScript")
	options.debug = true

	# Set up event callbacks
	options.before_send = _before_send
	options.on_crash = _on_crash


## before_send callback example
func _before_send(ev: SentryEvent) -> SentryEvent:
	print("[example_configuration.gd] Processing event: ", ev.id)
	return ev


## on_crash callback example
func _on_crash(ev: SentryEvent) -> SentryEvent:
	print("[example_configuration.gd] Crashing with event: ", ev.id)
	return ev
