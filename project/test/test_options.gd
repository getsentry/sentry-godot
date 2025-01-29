class_name TestOptions
extends GdUnitTestSuite


## SentryOptions.enabled should be set to the specified value.
func test_enabled() -> void:
	var options := SentryOptions.new()
	options.enabled = true
	assert_bool(options.enabled).is_true()
	options.enabled = false
	assert_bool(options.enabled).is_false()
	#await get_tree().process_frame
	#assert_bool(SentrySDK.is_enabled()).is_false()


## SentryOptions.disabled_in_editor should be set to the specified value.
func test_disabled_in_editor() -> void:
	var options := SentryOptions.new()
	options.disabled_in_editor = true
	assert_bool(options.disabled_in_editor).is_true()
	options.disabled_in_editor = false
	assert_bool(options.disabled_in_editor).is_false()


## SentryOptions.dsn should be set to the specified value.
func test_dsn() -> void:
	var options := SentryOptions.new()
	var some_dsn := "https://abcd@o1234.ingest.example.com/12345678"
	options.dsn = some_dsn
	assert_str(options.dsn).is_equal(some_dsn)


## SentryOptions.release should be set to the specified value.
func test_release() -> void:
	var options := SentryOptions.new()
	options.release = "1.2.3"
	assert_str(options.release).is_equal("1.2.3")


## SentryOptions.debug should be set to the specified value.
func test_debug() -> void:
	var options := SentryOptions.new()
	options.debug = true
	assert_bool(options.debug).is_true()
	options.debug = false
	assert_bool(options.debug).is_false()


## SentryOptions.environment should be set to the specified value.
func test_environment() -> void:
	var options := SentryOptions.new()
	options.environment = "test-environment"
	assert_str(options.environment).is_equal("test-environment")


## SentryOptions.sample_rate should be set to the specified value.
func test_sample_rate() -> void:
	var options := SentryOptions.new()
	options.sample_rate = 0.5
	assert_float(options.sample_rate).is_equal_approx(0.5, 0.01)


## SentryOptions.attach_log should be set to the specified value.
func test_attach_log() -> void:
	var options := SentryOptions.new()
	options.attach_log = true
	assert_bool(options.attach_log).is_true()
	options.attach_log = false
	assert_bool(options.attach_log).is_false()


## SentryOptions.max_breadcrumbs should be set to the specified value.
func test_max_breadcrumbs() -> void:
	var options := SentryOptions.new()
	options.max_breadcrumbs = 42
	assert_int(options.max_breadcrumbs).is_equal(42)


## SentryOptions.send_default_pii should be set to the specified value.
func test_send_default_pii() -> void:
	var options := SentryOptions.new()
	options.send_default_pii = true
	assert_bool(options.send_default_pii).is_true()
	options.send_default_pii = false
	assert_bool(options.send_default_pii).is_false()
