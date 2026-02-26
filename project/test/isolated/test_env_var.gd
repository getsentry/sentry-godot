extends SentryTestSuite
## Test that env vars are applied when project settings are at their defaults.


const TEST_DSN := "https://examplePublicKey@o0.ingest.sentry.io/0"


func init_sdk() -> void:
	ProjectSettings.set_setting("sentry/options/dsn", "")
	OS.set_environment("SENTRY_DSN", TEST_DSN)
	OS.set_environment("SENTRY_RELEASE", "env-release@1.0")
	OS.set_environment("SENTRY_ENVIRONMENT", "env-testing")
	SentrySDK.init()


func after() -> void:
	super()
	OS.unset_environment("SENTRY_DSN")
	OS.unset_environment("SENTRY_RELEASE")
	OS.unset_environment("SENTRY_ENVIRONMENT")


## SENTRY_DSN should be used to initialize the SDK.
func test_env_var_dsn() -> void:
	assert_bool(SentrySDK.is_enabled()).is_true()


## SENTRY_RELEASE should be used as the release value.
func test_env_var_release() -> void:
	var json := await capture_event_and_get_json(SentrySDK.create_event())
	assert_json(json).at("release").must_be("env-release@1.0").verify()


## SENTRY_ENVIRONMENT should be used as the environment value.
func test_env_var_environment() -> void:
	var json := await capture_event_and_get_json(SentrySDK.create_event())
	assert_json(json).at("environment").must_be("env-testing").verify()
