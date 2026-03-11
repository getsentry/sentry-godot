extends SentryTestSuite
## Test that the init callback takes precedence over env vars.


func init_sdk() -> void:
	OS.set_environment("SENTRY_RELEASE", "env-release@1.0")
	OS.set_environment("SENTRY_ENVIRONMENT", "env-testing")
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.release = "code-release@3.0"
		options.environment = "code-env"
	)


func after() -> void:
	super()
	OS.unset_environment("SENTRY_RELEASE")
	OS.unset_environment("SENTRY_ENVIRONMENT")


## Release set in init callback should take precedence over SENTRY_RELEASE.
func test_code_overrides_env_var_release() -> void:
	var json := await capture_event_and_get_json(SentrySDK.create_event())
	assert_json(json).at("release").must_be("code-release@3.0").verify()


## Environment set in init callback should take precedence over SENTRY_ENVIRONMENT.
func test_code_overrides_env_var_environment() -> void:
	var json := await capture_event_and_get_json(SentrySDK.create_event())
	assert_json(json).at("environment").must_be("code-env").verify()
