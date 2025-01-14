extends SentryConfiguration

## Setting up filtering
func _initialize() -> void:
	# before-send example:
	SentrySDK.set_before_send(
		func(ev: SentryEvent) -> SentryEvent:
			print("Processing event: ", ev.id)
			return ev
	)

	# on-crash example:
	SentrySDK.set_on_crash(
		func(ev: SentryEvent) -> SentryEvent:
			print("Crashing with event: ", ev.id)
			return ev
	)
