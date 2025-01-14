extends SentryConfiguration

## Sets up filtering
func _initialize() -> void:
	SentrySDK.set_before_send(_before_send)
	SentrySDK.set_on_crash(_on_crash)


## before_send example
func _before_send(ev: SentryEvent) -> SentryEvent:
	print("Processing event: ", ev.id)
	return ev


## on_crash example
func _on_crash(ev: SentryEvent) -> SentryEvent:
	print("Crashing with event: ", ev.id)
	return ev
