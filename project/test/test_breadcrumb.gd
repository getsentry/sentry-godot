extends GdUnitTestSuite
## Test SentryBreadcrumb class.


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


# TODO: implement data!
# func test_breadcrumb_data() -> void:
#     crumb.data = {"test": "data"}
#     assert_dict(crumb.data).is_equal({"test": "data"})


# TODO: implement timestamp!
# func test_breadcrumb_timestamp() -> void:
#     crumb.timestamp = 123.456
#     assert_float(crumb.timestamp).is_equal(123.456)
