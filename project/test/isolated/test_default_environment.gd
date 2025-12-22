extends SentryTestSuite

func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.before_send = _before_send
	)


func _before_send(event: SentryEvent) -> SentryEvent:
	assert_bool(event.environment in [
		"editor_dev",
		"editor_dev_run",
		"export_debug",
		"export_release",
		"dedicated_server"]).override_failure_message(
				"Environment should be automatically detected").is_true()
	return null


func test_default_environment() -> void:
	SentrySDK.capture_event(SentrySDK.create_event())
