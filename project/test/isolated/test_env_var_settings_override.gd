extends SentryTestSuite
## Test that explicit project settings take precedence over env vars.


func init_sdk() -> void:
	OS.set_environment("SENTRY_RELEASE", "env-release@1.0")
	OS.set_environment("SENTRY_ENVIRONMENT", "env-testing")
	ProjectSettings.set_setting("sentry/options/release", "settings-release@2.0")
	ProjectSettings.set_setting("sentry/options/environment", "settings-env")
	SentrySDK.init()


func after() -> void:
	super()
	OS.unset_environment("SENTRY_RELEASE")
	OS.unset_environment("SENTRY_ENVIRONMENT")

	# Restore settings
	var defaults := SentryOptions.new()
	ProjectSettings.set_setting("sentry/options/release", defaults.release)
	ProjectSettings.set_setting("sentry/options/environment", defaults.environment)


## Explicit release project setting should take precedence over SENTRY_RELEASE.
func test_project_setting_overrides_env_var_release() -> void:
	var json := await capture_event_and_get_json(SentrySDK.create_event())
	assert_json(json).at("release").must_be("settings-release@2.0").verify()


## Explicit environment project setting should take precedence over SENTRY_ENVIRONMENT.
func test_project_setting_overrides_env_var_environment() -> void:
	var json := await capture_event_and_get_json(SentrySDK.create_event())
	assert_json(json).at("environment").must_be("settings-env").verify()
