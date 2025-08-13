extends GutTest
## Basic tests for the SentryOptions class.


var options: SentryOptions


func before_each() -> void:
	options = SentryOptions.new()


## Test simple bool properties.
func test_bool_properties(property=use_parameters([
		"enabled",
		"disabled_in_editor_play",
		"debug",
		"attach_log",
		"attach_screenshot",
		"attach_scene_tree",
		"send_default_pii",
		"logger_enabled",
		"logger_include_source",
])) -> void:
	options.set(property, true)
	assert_true(options.get(property))
	options.set(property, false)
	assert_false(options.get(property))


## Test simple string properties.
func test_string_properties(property=use_parameters([
		"dsn",
		"release",
		"dist",
		"environment",
])) -> void:
	options.set(property, "test-value")
	assert_eq(options.get(property), "test-value")


## SentryOptions.sample_rate should be set to the specified value.
func test_sample_rate() -> void:
	options.sample_rate = 0.5
	assert_almost_eq(options.sample_rate, 0.5, 0.01)


## SentryOptions.max_breadcrumbs should be set to the specified value.
func test_max_breadcrumbs() -> void:
	options.max_breadcrumbs = 42
	assert_eq(options.max_breadcrumbs, 42)


## SentryOptions.logger_event_mask should be set to the specified value.
func test_logger_event_mask() -> void:
	var mask := SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER
	options.logger_event_mask = mask
	assert_eq(options.logger_event_mask, mask)


## SentryOptions.logger_breadcrumb_mask should be set to the specified value.
func test_logger_breadcrumb_mask() -> void:
	var mask := SentryOptions.MASK_SCRIPT | SentryOptions.MASK_SHADER
	options.logger_breadcrumb_mask = mask
	assert_eq(options.logger_breadcrumb_mask, mask)


## Test integer error logger limit properties.
func test_logger_limit_properties(property=use_parameters([
		"events_per_frame",
		"repeated_error_window_ms",
		"throttle_events",
		"throttle_window_ms",
])) -> void:
	options.logger_limits.set(property, 42)
	assert_eq(options.logger_limits.get(property), 42)


## Test properties with SentrySDK.Level type.
func test_level_properties(property=use_parameters([
		"diagnostic_level",
		"screenshot_level"
])) -> void:
	var prev: SentrySDK.Level = options.get(property)
	options.set(property, SentrySDK.LEVEL_WARNING)
	assert_eq(options.get(property), SentrySDK.LEVEL_WARNING)
	options.set(property, prev)
	assert_eq(options.get(property), prev)


## Test assigning various callback properties.
func test_callback_properties(property=use_parameters([
		"before_send",
		"before_capture_screenshot"
])) -> void:
	var callback := func(_a1): pass
	var prev: Callable = options.get(property)
	options.set(property, callback)
	assert_eq(options.get(property), callback)
	options.set(property, prev)
