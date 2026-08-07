extends SentryTestSuite
## Test lifecycle methods.


func before() -> void:
	# NOTE: Not calling super() on purpose - this suite initializes the SDK itself.
	pass


func _trace_id(json: String) -> Variant:
	var data: Variant = JSON.parse_string(json)
	return data.get("contexts", {}).get("trace", {}).get("trace_id")


## Test manual initialization and shutdown of SDK.
func test_sdk_lifecycle() -> void:
	# SDK should be disabled at start.
	assert_bool(SentrySDK.is_enabled()).is_false()

	SentrySDK.capture_message("message not captured before SDK is initialized")
	await assert_signal(self).is_not_emitted("event_captured")

	SentrySDK.init(func (options: SentryOptions) -> void:
		options.before_send = _before_send
		options.shutdown_timeout_ms = 2000
	)
	assert_bool(SentrySDK.is_enabled()).is_true()

	SentrySDK.capture_message("message captured when SDK is initialiazed")
	await assert_signal(self).is_emitted("event_captured")

	SentrySDK.close()

	# NOTE: On Web, Sentry.close() is async - need to wait for it to complete.
	#       isEnabled only updates after flushing is finished.
	await get_tree().create_timer(2.0).timeout
	assert_bool(SentrySDK.is_enabled()).is_false()

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

	SentrySDK.set_tag("session", "first")
	SentrySDK.add_breadcrumb(SentryBreadcrumb.create("first session breadcrumb"))
	SentrySDK.capture_message("message from the first session")
	var first_json: String = await wait_for_captured_event_json()

	assert_json(first_json).describe("First session event carries its tag") \
		.must_contain("/tags/session", "first") \
		.verify()

	assert_json(first_json).describe("First session event carries its breadcrumb") \
		.at("/breadcrumbs/") \
		.with_objects() \
		.containing("message", "first session breadcrumb") \
		.exactly(1)

	assert_json(first_json).describe("First session event is on a trace") \
		.at("/contexts/trace/trace_id") \
		.is_not_empty() \
		.verify()

	var first_trace_id: Variant = _trace_id(first_json)

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

	assert_json(second_json).describe("Second session drops the first session's tag") \
		.must_not_contain("/tags/session") \
		.verify()

	assert_str(second_json).not_contains("first session breadcrumb")

	assert_json(second_json).describe("Second session starts a new trace") \
		.at("/contexts/trace/trace_id") \
		.is_not_equal(first_trace_id) \
		.verify()

	SentrySDK.close()
