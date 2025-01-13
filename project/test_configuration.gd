extends SentryConfiguration

func _initialize() -> void:
	SentrySDK.set_before_send(
		func(ev: SentryEvent) -> SentryEvent:
			# override level
			print("OVERRIDING LEVEL")
			ev.level = SentrySDK.LEVEL_DEBUG
			return ev
	)
