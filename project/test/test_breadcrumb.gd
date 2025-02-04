extends GdUnitTestSuite
## Test SentryBreadcrumb class.


signal callback_processed

var crumb: SentryBreadcrumb


func before_test() -> void:
	crumb = SentrySDK.create_breadcrumb()


func test_breadcrumb_message() -> void:
	crumb.message = "test-message"
	assert_str(crumb.message).is_equal("test-message")


func test_breadcrumb_category() -> void:
	crumb.category = "test-category"
	assert_str(crumb.category).is_equal("test-category")


func test_breadcrumb_level() -> void:
	crumb.level = SentrySDK.LEVEL_DEBUG
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_DEBUG)


func test_breadcrumb_type() -> void:
	crumb.type = "test-type"
	assert_str(crumb.type).is_equal("test-type")


func test_with_before_breadcrumb() -> void:
	SentrySDK._set_before_breadcrumb(
		func(b: SentryBreadcrumb):
			assert_str(b.message).is_equal("test-message")
			assert_str(b.category).is_equal("test-category")
			assert_int(b.level).is_equal(SentrySDK.LEVEL_DEBUG)
			assert_str(b.type).is_equal("test-type")
			assert_dict(b.get_data()).is_equal({"test": "data"})
			assert_str(b.timestamp).is_equal("2025-01-01T20:00:00+00:00")
			callback_processed.emit()
			return null # discard breadcrumb
	)

	var monitor := monitor_signals(self, false)

	crumb.message = "test-message"
	crumb.category = "test-category"
	crumb.level = SentrySDK.LEVEL_DEBUG
	crumb.type = "test-type"
	crumb.set_data({"test": "data"})
	crumb.timestamp = "2025-01-01T20:00:00+00:00"
	SentrySDK.capture_breadcrumb(crumb)

	await assert_signal(monitor).is_emitted("callback_processed")

	SentrySDK._unset_before_breadcrumb()


func test_breadcrumb_data() -> void:
	crumb.set_data({"str": "text", "int": 42})
	assert_dict(crumb.get_data()).is_equal({"str": "text", "int": 42})


func test_breadcrumb_timestamp() -> void:
	var ts = Time.get_datetime_string_from_system()
	crumb.timestamp = ts
	assert_str(crumb.timestamp).is_equal(ts)
