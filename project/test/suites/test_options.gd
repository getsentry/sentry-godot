extends GdUnitTestSuite
## Basic tests for the SentryOptions class.


var options: SentryOptions


func before_test() -> void:
	options = SentryOptions.new()


## Test simple bool properties.
@warning_ignore("unused_parameter")
func test_bool_properties(property: String, test_parameters := [
		["enabled"],
		["disabled_in_editor"],
		["debug"],
		["attach_log"],
		["send_default_pii"],
		["error_logger_enabled"],
		["error_logger_include_source"],
]) -> void:
	options.set(property, true)
	assert_bool(options.get(property)).is_true()
	options.set(property, false)
	assert_bool(options.get(property)).is_false()


## Test simple string properties.
@warning_ignore("unused_parameter")
func test_string_properties(property: String, test_parameters := [
		["dsn"],
		["release"],
		["dist"],
		["environment"],
]) -> void:
	options.set(property, "test-value")
	assert_str(options.get(property)).is_equal("test-value")


## SentryOptions.sample_rate should be set to the specified value.
func test_sample_rate() -> void:
	options.sample_rate = 0.5
	assert_float(options.sample_rate).is_equal_approx(0.5, 0.01)


## SentryOptions.max_breadcrumbs should be set to the specified value.
func test_max_breadcrumbs() -> void:
	options.max_breadcrumbs = 42
	assert_int(options.max_breadcrumbs).is_equal(42)


## SentryOptions.screenshot_event_types must be set to the specified value.
func test_screenshot_event_types() -> void:
	options.screenshot_event_types = SentryEvent.EVENT_MASK_CRASH_EVENTS | SentryEvent.EVENT_MASK_CUSTOM_EVENTS
	assert_int(options.screenshot_event_types & SentryEvent.EVENT_MASK_CRASH_EVENTS).is_not_zero()
	assert_int(options.screenshot_event_types & SentryEvent.EVENT_MASK_CUSTOM_EVENTS).is_not_zero()
	assert_int(options.screenshot_event_types & SentryEvent.EVENT_MASK_ERROR_EVENTS).is_zero()


## SentryOptions.error_logger_event_mask should be set to the specified value.
func test_error_logger_event_mask() -> void:
	var mask := SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER
	options.error_logger_event_mask = mask
	assert_int(options.error_logger_event_mask).is_equal(mask)


## SentryOptions.error_logger_breadcrumb_mask should be set to the specified value.
func test_error_logger_breadcrumb_mask() -> void:
	var mask := SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER
	options.error_logger_breadcrumb_mask = mask
	assert_int(options.error_logger_breadcrumb_mask).is_equal(mask)


## Test integer error logger limit properties.
@warning_ignore("unused_parameter")
func test_error_logger_limit_properties(property: String, test_parameters := [
		["events_per_frame"],
		["repeated_error_window_ms"],
		["throttle_events"],
		["throttle_window_ms"],
]) -> void:
	options.error_logger_limits.set(property, 42)
	assert_int(options.error_logger_limits.get(property)).is_equal(42)
