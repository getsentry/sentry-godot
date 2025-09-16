extends SentryTestSuite
## Basic tests for the SentryOptions class.


var options: SentryOptions


func before_test() -> void:
	super()
	options = SentryOptions.new()


## Test simple bool properties.
@warning_ignore("unused_parameter")
func test_bool_properties(property: String, test_parameters := [
		["debug"],
		["attach_log"],
		["attach_screenshot"],
		["attach_scene_tree"],
		["send_default_pii"],
		["logger_enabled"],
		["logger_include_source"],
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


## SentryOptions.logger_event_mask should be set to the specified value.
func test_logger_event_mask() -> void:
	var mask := SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER
	options.logger_event_mask = mask
	assert_int(options.logger_event_mask).is_equal(mask)


## SentryOptions.logger_breadcrumb_mask should be set to the specified value.
func test_logger_breadcrumb_mask() -> void:
	var mask := SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER
	options.logger_breadcrumb_mask = mask
	assert_int(options.logger_breadcrumb_mask).is_equal(mask)


## Test integer error logger limit properties.
@warning_ignore("unused_parameter")
func test_logger_limit_properties(property: String, test_parameters := [
		["events_per_frame"],
		["repeated_error_window_ms"],
		["throttle_events"],
		["throttle_window_ms"],
]) -> void:
	options.logger_limits.set(property, 42)
	assert_int(options.logger_limits.get(property)).is_equal(42)


## Test properties with SentrySDK.Level type.
@warning_ignore("unused_parameter")
func test_level_properties(property: String, test_parameters := [
	["diagnostic_level"],
	["screenshot_level"]
]) -> void:
	var prev: SentrySDK.Level = options.get(property)
	options.set(property, SentrySDK.LEVEL_WARNING)
	assert_int(options.get(property)).is_equal(SentrySDK.LEVEL_WARNING)
	options.set(property, prev)
	assert_int(options.get(property)).is_equal(prev)


## Test assigning various callback properties.
@warning_ignore("unused_parameter")
func test_callback_properties(property: String, test_parameters := [
	["before_send"],
	["before_capture_screenshot"]
]) -> void:
	var callback := func(_a): pass
	var prev: Callable = options.get(property)
	options.set(property, callback)
	assert_that(options.get(property)).is_equal(callback)
	options.set(property, prev)
