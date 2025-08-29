extends GdUnitTestSuite
## Test SentryBreadcrumb class properties and methods.


func test_breadcrumb_message() -> void:
	var crumb: SentryBreadcrumb = SentrySDK.create_breadcrumb()
	crumb.message = "test-message"
	assert_str(crumb.message).is_equal("test-message")


func test_breadcrumb_category() -> void:
	var crumb: SentryBreadcrumb = SentrySDK.create_breadcrumb()
	crumb.category = "test-category"
	assert_str(crumb.category).is_equal("test-category")


func test_breadcrumb_level() -> void:
	var crumb: SentryBreadcrumb = SentrySDK.create_breadcrumb()
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
	var crumb: SentryBreadcrumb = SentrySDK.create_breadcrumb()
	crumb.type = "test-type"
	assert_str(crumb.type).is_equal("test-type")


func test_breadcrumb_info_shortcut() -> void:
	var crumb := SentryBreadcrumb.info("Info message")
	assert_str(crumb.message).is_equal("Info message")
	assert_str(crumb.type).is_equal("info")
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_INFO)
	assert_str(crumb.category).is_equal("default")


func test_breadcrumb_debug_shortcut() -> void:
	var crumb := SentryBreadcrumb.debug("Debug message")
	assert_str(crumb.message).is_equal("Debug message")
	assert_str(crumb.type).is_equal("debug")
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_DEBUG)
	assert_str(crumb.category).is_equal("default")


func test_breadcrumb_error_shortcut() -> void:
	var crumb := SentryBreadcrumb.error("Error happened")
	assert_str(crumb.message).is_equal("Error happened")
	assert_str(crumb.type).is_equal("error")
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_ERROR)
	assert_str(crumb.category).is_equal("default")


func test_breadcrumb_query_shortcut() -> void:
	var crumb := SentryBreadcrumb.query("Query performed")
	assert_str(crumb.message).is_equal("Query performed")
	assert_str(crumb.type).is_equal("query")
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_INFO)
	assert_str(crumb.category).is_equal("default")


func test_breadcrumb_user_shortcut() -> void:
	var crumb := SentryBreadcrumb.user("ui.click", "User clicked button")
	assert_str(crumb.message).is_equal("User clicked button")
	assert_str(crumb.type).is_equal("user")
	assert_int(crumb.level).is_equal(SentrySDK.LEVEL_INFO)
	assert_str(crumb.category).is_equal("ui.click")
