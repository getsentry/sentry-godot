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
]) -> void:
	options.set(property, true)
	assert_bool(options.get(property)).is_true()
	options.set(property, false)
	assert_bool(options.get(property)).is_false()


## Test simple bool properties on godot_logger options.
@warning_ignore("unused_parameter")
func test_godot_logger_bool_properties(property: String, test_parameters := [
		["enabled"],
		["include_source_context"],
		["include_variables"],
		["enable_capture_during_shutdown"],
]) -> void:
	options.godot_logger.set(property, true)
	assert_bool(options.godot_logger.get(property)).is_true()
	options.godot_logger.set(property, false)
	assert_bool(options.godot_logger.get(property)).is_false()


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


## SentryOptions.shutdown_timeout_ms should be set to the specified value.
func test_shutdown_timeout_ms() -> void:
	options.shutdown_timeout_ms = 5000
	assert_int(options.shutdown_timeout_ms).is_equal(5000)


## Test mask properties on godot_logger options.
@warning_ignore("unused_parameter")
func test_godot_logger_mask_properties(property: String, test_parameters := [
		["event_mask"],
		["breadcrumb_mask"],
		["log_mask"],
]) -> void:
	var mask := SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER
	options.godot_logger.set(property, mask)
	assert_int(options.godot_logger.get(property)).is_equal(mask)


## Test integer error logger limit properties.
@warning_ignore("unused_parameter")
func test_logger_limit_properties(property: String, test_parameters := [
		["events_per_frame"],
		["repeated_error_window_ms"],
		["throttle_events"],
		["throttle_window_ms"],
]) -> void:
	options.godot_logger.limits.set(property, 42)
	assert_int(options.godot_logger.limits.get(property)).is_equal(42)


## Deprecated flat logger_* properties should proxy to godot_logger options.
@warning_ignore("unused_parameter")
func test_deprecated_logger_properties(property: String, new_property: String, value: Variant, test_parameters := [
		["logger_enabled", "enabled", false],
		["logger_include_source", "include_source_context", false],
		["logger_include_variables", "include_variables", true],
		["logger_event_mask", "event_mask", SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER],
		["logger_breadcrumb_mask", "breadcrumb_mask", SentryOptions.MASK_SCRIPT],
		["logger_log_mask", "log_mask", SentryOptions.MASK_ERROR],
]) -> void:
	options.set(property, value)
	assert_that(options.godot_logger.get(new_property)).is_equal(value)
	assert_that(options.get(property)).is_equal(value)


## Deprecated logger_messages_as_breadcrumbs should toggle MASK_MESSAGE in godot_logger.breadcrumb_mask.
func test_deprecated_logger_messages_as_breadcrumbs() -> void:
	options.logger_messages_as_breadcrumbs = false
	assert_int(options.godot_logger.breadcrumb_mask & SentryOptions.MASK_MESSAGE).is_equal(0)
	options.logger_messages_as_breadcrumbs = true
	assert_int(options.godot_logger.breadcrumb_mask & SentryOptions.MASK_MESSAGE).is_equal(SentryOptions.MASK_MESSAGE)


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
