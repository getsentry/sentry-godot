extends GdUnitTestSuite
## Test SentryBreadcrumb class properties and methods.


func test_breadcrumb_create_with_message() -> void:
	var crumb := SentryBreadcrumb.create("test-message")
	assert_str(crumb.message).is_equal("test-message")


func test_breadcrumb_message() -> void:
	var crumb := SentryBreadcrumb.create()
	crumb.message = "test-message"
	assert_str(crumb.message).is_equal("test-message")


func test_breadcrumb_category() -> void:
	var crumb := SentryBreadcrumb.create()
	crumb.category = "test-category"
	assert_str(crumb.category).is_equal("test-category")


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


func test_breadcrumb_type() -> void:
	var crumb := SentryBreadcrumb.create()
	crumb.type = "test-type"
	assert_str(crumb.type).is_equal("test-type")


func test_breadcrumb_can_set_data() -> void:
	var crumb := SentryBreadcrumb.create()
	crumb.set_data({"biome": "forest", "time_of_day": 0.42})


func test_breadcrumb_default_values() -> void:
	var crumb := SentryBreadcrumb.create()
	assert_str(crumb.message).is_empty()
	assert_str(crumb.type).is_empty()
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_INFO)
	assert_bool(crumb.category in ["", "default"]).is_true()
