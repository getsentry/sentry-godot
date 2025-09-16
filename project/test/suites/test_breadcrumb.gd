extends SentryTestSuite
## Test SentryBreadcrumb class properties and methods.


## Test string properties accessors and UTF-8 encoding preservation.
@warning_ignore("unused_parameter")
func test_string_properties_and_utf8(property: String, test_parameters := [
		["message"],
		["category"],
		["type"],
]) -> void:
	var crumb := SentryBreadcrumb.create()
	crumb.set(property, "Hello, World!")
	assert_str(crumb.get(property)).is_equal("Hello, World!")
	crumb.set(property, "Hello 世界! 👋")
	assert_str(crumb.get(property)).is_equal("Hello 世界! 👋")


func test_breadcrumb_create_with_message() -> void:
	var crumb := SentryBreadcrumb.create("test-message")
	assert_str(crumb.message).is_equal("test-message")


func test_breadcrumb_level() -> void:
	var crumb := SentryBreadcrumb.create()
	crumb.level = SentrySDK.LEVEL_DEBUG
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_DEBUG)
	crumb.level = SentrySDK.LEVEL_INFO
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_INFO)
	crumb.level = SentrySDK.LEVEL_WARNING
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_WARNING)
	crumb.level = SentrySDK.LEVEL_ERROR
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_ERROR)
	crumb.level = SentrySDK.LEVEL_FATAL
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_FATAL)


func test_breadcrumb_can_set_data() -> void:
	var crumb := SentryBreadcrumb.create()
	crumb.set_data({"biome": "forest", "time_of_day": 0.42})


func test_breadcrumb_default_values() -> void:
	var crumb := SentryBreadcrumb.create()
	assert_str(crumb.message).is_empty()
	assert_str(crumb.type).is_empty()
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_INFO)
	assert_bool(crumb.category in ["", "default"]).is_true()


func test_breadcrumb_timestamp_is_set_automatically() -> void:
	const time_tolerance: float = 0.001  # 1 ms

	var time_before: float = Time.get_unix_time_from_system() - time_tolerance
	await get_tree().process_frame  # small delay to ensure timestamp differs

	var crumb := SentryBreadcrumb.create()

	await get_tree().process_frame
	var time_after: float = Time.get_unix_time_from_system() + time_tolerance

	# Timestamp should be between time_before and time_after
	var timestamp: SentryTimestamp = crumb.get_timestamp()
	assert_object(timestamp).is_not_null()
	var micros: int = timestamp.microseconds_since_unix_epoch
	var timestamp_unix: float = float(micros) / 1_000_000.0
	assert_float(timestamp_unix).is_greater_equal(time_before)
	assert_float(timestamp_unix).is_less_equal(time_after)
