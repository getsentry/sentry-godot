extends SentryTestSuite
## Test lifecycle methods.


func before() -> void:
	# NOTE: Not calling super() on purpose - this suite initializes the SDK itself.
	pass


func test_get_version_info_before_init() -> void:
	assert_bool(SentrySDK.is_enabled()).is_false()
	var version: Dictionary = SentrySDK.get_version_info()
	assert_int(version.size()).is_equal(4)
	assert_str(version["version"]).is_not_empty()
	assert_str(version["commit"]).is_not_empty()
	if OS.has_feature("linux") or OS.has_feature("windows"):
		assert_str(version["backend"]).is_equal("sentry-native")
		var expected_variant: String = "sentry_crash_backend_crashpad"
		if OS.has_feature("sentry_crash_backend_native"):
			expected_variant = "sentry_crash_backend_native"
		assert_str(version["variant"]).is_equal(expected_variant)
	else:
		var backends: Dictionary = {
			"macos": "sentry-cocoa",
			"ios": "sentry-cocoa",
			"android": "sentry-android",
			"web": "sentry-javascript",
		}
		for feature: String in backends:
			if OS.has_feature(feature):
				assert_str(version["backend"]).is_equal(backends[feature])
		assert_str(version["variant"]).is_empty()
	version["backend"] = "modified"
	assert_str(SentrySDK.get_version_info()["backend"]).is_not_equal("modified")


## Test manual initialization and shutdown of SDK.
func test_sdk_lifecycle() -> void:
	# SDK should be disabled at start.
	assert_bool(SentrySDK.is_enabled()).is_false()
	var version: Dictionary = SentrySDK.get_version_info()

	SentrySDK.capture_message("message not captured before SDK is initialized")
	await assert_signal(self).is_not_emitted("event_captured")

	SentrySDK.init(func (options: SentryOptions) -> void:
		options.before_send = _before_send
		options.shutdown_timeout_ms = 2000
	)
	assert_bool(SentrySDK.is_enabled()).is_true()
	assert_dict(SentrySDK.get_version_info()).is_equal(version)

	SentrySDK.capture_message("message captured when SDK is initialiazed")
	await assert_signal(self).is_emitted("event_captured")

	SentrySDK.close()

	# NOTE: On Web, Sentry.close() is async - need to wait for it to complete.
	#       isEnabled only updates after flushing is finished.
	await get_tree().create_timer(2.0).timeout
	assert_bool(SentrySDK.is_enabled()).is_false()
	assert_dict(SentrySDK.get_version_info()).is_equal(version)

	SentrySDK.capture_message("message not captured when SDK is closed")
	await assert_signal(self).is_not_emitted("event_captured")


## Test that re-init creates fresh options (old before_send should not leak).
func test_reinit_clears_options() -> void:
	# First init with before_send callback.
	SentrySDK.init(func (options: SentryOptions) -> void:
		options.before_send = _before_send
		options.shutdown_timeout_ms = 2000
	)
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message triggers before_send")
	await assert_signal(self).is_emitted("event_captured")

	SentrySDK.close()

	# NOTE: On Web, Sentry.close() is async - need to wait for it to complete.
	#       isEnabled only updates after flushing is finished.
	await get_tree().create_timer(2.0).timeout

	# Re-init WITHOUT callback -- old before_send should be gone.
	SentrySDK.init()
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message should not trigger old before_send")
	await assert_signal(self).is_not_emitted("event_captured")

	SentrySDK.close()

	# NOTE: On Web, Sentry.close() is async - let it finish before the next test initializes.
	await get_tree().create_timer(2.0).timeout


## Test that re-init clears globally set data (old tags and breadcrumbs should not leak).
func test_reinit_clears_global_data() -> void:
	SentrySDK.init(func (options: SentryOptions) -> void:
		options.before_send = _before_send
		options.shutdown_timeout_ms = 2000
	)

	var user := SentryUser.new()
	user.id = "first-session-user"
	SentrySDK.set_user(user)
	SentrySDK.set_tag("session", "first")
	SentrySDK.set_context("session", {"name": "first"})
	SentrySDK.add_breadcrumb(SentryBreadcrumb.create("first session breadcrumb"))
	SentrySDK.capture_message("message from the first session")
	var first_json: String = await wait_for_captured_event_json()

	assert_json(first_json).describe("First session event carries its user") \
		.must_contain("/user/id", "first-session-user") \
		.verify()

	assert_json(first_json).describe("First session event carries its tag") \
		.must_contain("/tags/session", "first") \
		.verify()

	assert_json(first_json).describe("First session event carries its context") \
		.must_contain("/contexts/session/name", "first") \
		.verify()

	var first_event: Dictionary = JSON.parse_string(first_json)
	var first_trace_id: String = first_event.get("contexts", {}).get("trace", {}).get("trace_id", "")
	assert_str(first_trace_id).is_not_empty()

	assert_json(first_json).describe("First session event carries its breadcrumb") \
		.at("/breadcrumbs/") \
		.with_objects() \
		.containing("message", "first session breadcrumb") \
		.exactly(1)

	SentrySDK.close()

	# NOTE: On Web, Sentry.close() is async - need to wait for it to complete.
	await get_tree().create_timer(2.0).timeout

	# Re-init without setting them again -- the data from the first session should be gone.
	SentrySDK.init(func (options: SentryOptions) -> void:
		options.before_send = _before_send
		options.shutdown_timeout_ms = 2000
	)

	SentrySDK.capture_message("message from the second session")
	var second_json: String = await wait_for_captured_event_json()

	assert_json(second_json).describe("Second session drops the first session's user") \
		.must_not_contain("/user/id", "first-session-user") \
		.verify()

	assert_json(second_json).describe("Second session drops the first session's tag") \
		.must_not_contain("/tags/session") \
		.verify()

	assert_json(second_json).describe("Second session drops the first session's context") \
		.must_not_contain("/contexts/session") \
		.verify()

	assert_str(second_json).not_contains("first session breadcrumb")

	var second_event: Dictionary = JSON.parse_string(second_json)
	var second_trace_id: String = second_event.get("contexts", {}).get("trace", {}).get("trace_id", "")
	assert_str(second_trace_id).is_not_empty().is_not_equal(first_trace_id)

	SentrySDK.close()
