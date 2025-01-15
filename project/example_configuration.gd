extends SentryConfiguration
## Example Sentry configuration script.
##
## Tip: You can assign configuration script in the project settings.


## Initialize filtering
func _initialize(options: SentryOptions) -> void:
	options.debug = false
	# Set up before_send and on_crash callbacks
	options.before_send = _before_send
	options.on_crash = _on_crash


## before_send callback example
func _before_send(ev: SentryEvent) -> SentryEvent:
	print("Processing event: ", ev.id)
	return ev


## on_crash callback example
func _on_crash(ev: SentryEvent) -> SentryEvent:
	print("Crashing with event: ", ev.id)
	return ev
