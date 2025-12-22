extends SentryTestSuite
## Test SentryOptions defaults with init.


var before_send: Callable # (event: SentryEvent) -> SentryEvent


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.before_send = _before_send
	)


func _before_send(event: SentryEvent) -> SentryEvent:
	return before_send.call(event)


func test_default_environment() -> void:
	before_send = func(event: SentryEvent) -> SentryEvent:
		assert_bool(event.environment in [
				"editor_dev",
				"editor_dev_run",
				"export_debug",
				"export_release",
				"dedicated_server"
			]).override_failure_message(
				"Environment should be automatically detected"
			).is_true()
		return null

	SentrySDK.capture_event(SentrySDK.create_event())


func test_default_release() -> void:
	before_send = func(event: SentryEvent) -> SentryEvent:
		var expected := "%s@%s" % [
				ProjectSettings.get_setting("application/config/name"),
				ProjectSettings.get_setting("application/config/version")
			]
		assert_str(event.release).is_equal(expected)
		return null

	SentrySDK.capture_event(SentrySDK.create_event())
