extends GdUnitTestSuite
## Regression test for #797: a GDScript lambda set as a Sentry callback could crash on exit.
## This suite is isolated, so the runner catches such a crash via the exit code.


func test_lambda_callback_does_not_crash_on_exit() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.before_send = func(event: SentryEvent) -> SentryEvent:
			return event
	)
	assert_bool(SentrySDK.is_enabled()).is_true()
	# Leave the SDK open so the lambda is still set at shutdown, where the crash happened.
