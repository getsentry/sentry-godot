extends SentryTestSuite

signal log_processed(entry: SentryLog)

func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.enable_logs = true
		options.before_send_log = _before_send_log
	)


func _before_send_log(entry: SentryLog) -> SentryLog:
	log_processed.emit(entry)
	return entry


func test_structured_logs_with_print() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Test 123")
		assert_int(entry.level).is_equal(SentryLog.LOG_LEVEL_INFO)
	, CONNECT_ONE_SHOT)
	print("Test 123")


func test_structured_logs_with_printerr() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Test 123")
		assert_int(entry.level).is_equal(SentryLog.LOG_LEVEL_ERROR)
	, CONNECT_ONE_SHOT)
	printerr("Test 123")


func test_structured_logs_levels() -> void:
	for level in [
			SentryLog.LOG_LEVEL_TRACE,
			SentryLog.LOG_LEVEL_DEBUG,
			SentryLog.LOG_LEVEL_INFO,
			SentryLog.LOG_LEVEL_WARN,
			SentryLog.LOG_LEVEL_ERROR,
			SentryLog.LOG_LEVEL_FATAL]:
		log_processed.connect(func(entry: SentryLog):
			assert_int(entry.level).is_equal(level)
		, CONNECT_ONE_SHOT)
		SentrySDK.logger.log(level, "Test 123")


func test_structured_logs_shortcut_trace() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Test 123")
		assert_int(entry.level).is_equal(SentryLog.LOG_LEVEL_TRACE)
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.trace("Test 123")


func test_structured_logs_shortcut_debug() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Test 123")
		assert_int(entry.level).is_equal(SentryLog.LOG_LEVEL_DEBUG)
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.debug("Test 123")


func test_structured_logs_shortcut_info() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Test 123")
		assert_int(entry.level).is_equal(SentryLog.LOG_LEVEL_INFO)
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.info("Test 123")


func test_structured_logs_shortcut_warn() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Test 123")
		assert_int(entry.level).is_equal(SentryLog.LOG_LEVEL_WARN)
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.warn("Test 123")


func test_structured_logs_shortcut_error() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Test 123")
		assert_int(entry.level).is_equal(SentryLog.LOG_LEVEL_ERROR)
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.error("Test 123")


func test_structured_logs_shortcut_fatal() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Test 123")
		assert_int(entry.level).is_equal(SentryLog.LOG_LEVEL_FATAL)
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.fatal("Test 123")


func test_structured_logs_with_string_interpolation() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Test 123")
		assert_str(entry.get_attribute("sentry.message.template")).is_equal("%s %d")
		assert_str(entry.get_attribute("sentry.message.parameter.0")).is_equal("Test")
		assert_int(entry.get_attribute("sentry.message.parameter.1")).is_equal(123)
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.info("%s %d", ["Test", 123])


func test_structured_logs_with_custom_attributes() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.get_attribute("level")).is_equal("forest")
		assert_int(entry.get_attribute("enemy_id")).is_equal(42)
		assert_float(entry.get_attribute("health")).is_equal_approx(10.5, 0.001)
		assert_bool(entry.get_attribute("elite")).is_equal(false)
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.info("Test 123", [], {
		"level": "forest",
		"enemy_id": 42,
		"health": 10.5,
		"elite": false
	})


func test_structured_logs_with_attribute_removal() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.get_attribute("level")).is_equal("forest")
		entry.remove_attribute("level")
		assert_that(entry.get_attribute("level")).is_null()
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.info("Test 123", [], {
		"level": "forest",
	})


func test_structured_logs_with_utf8() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Hello 世界! 👋")
		assert_str(entry.get_attribute("sentry.message.template")).is_equal("Hello %s! 👋")
		assert_str(entry.get_attribute("sentry.message.parameter.0")).is_equal("世界")
		assert_str(entry.get_attribute("world")).is_equal("世界")
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.info("Hello %s! 👋", ["世界"], {
		"world": "世界"
	})

func test_structured_logs_attribute_methods() -> void:
	log_processed.connect(func(entry: SentryLog):
		entry.add_attributes({
			"hello": "世界", # string
			"meaning": 42 # integer
		})
		assert_str(entry.get_attribute("hello")).is_equal("世界")
		assert_int(entry.get_attribute("meaning")).is_equal(42)

		assert_str(entry.get_attribute("hello")).is_equal("世界")
		assert_int(entry.get_attribute("meaning")).is_equal(42)

		entry.set_attribute("test", true) # boolean
		assert_bool(entry.get_attribute("test")).is_true()

		entry.set_attribute("PI", 3.14) # double
		assert_float(entry.get_attribute("PI")).is_equal_approx(3.14, 0.001)

		entry.remove_attribute("hello")
		assert_that(entry.get_attribute("hello")).is_null()

		entry.remove_attribute("meaning")
		assert_that(entry.get_attribute("hello")).is_null()
		assert_that(entry.get_attribute("meaning")).is_null()
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.info("Test 123")


# Format specifiers in the body without arguments should be preserved as-is,
# not interpreted as a format string (which would cause a crash).
func test_structured_logs_body_with_format_specifiers() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Should preserve %n and %s and not crash")
		assert_that(entry.get_attribute("sentry.message.template")).is_null()
		assert_that(entry.get_attribute("sentry.message.parameter.0")).is_null()
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.info("Should preserve %n and %s and not crash")


# NOTE: JS SDK merges global scope attributes at serialization time, after beforeSendLog.
func test_structured_logs_with_global_attributes(_do_skip = OS.get_name() == "Web") -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_bool(entry.get_attribute("global_bool")).is_equal(true)
		assert_int(entry.get_attribute("global_int")).is_equal(42)
		assert_float(entry.get_attribute("global_float")).is_equal_approx(42.5, 0.001)
		assert_str(entry.get_attribute("global_string")).is_equal("string")
		assert_that(entry.get_attribute("deleted_attribute")).is_null()
	, CONNECT_ONE_SHOT)

	SentrySDK.set_attribute("global_bool", true)
	SentrySDK.set_attribute("global_int", 42)
	SentrySDK.set_attribute("global_float", 42.5)
	SentrySDK.set_attribute("global_string", "string")
	SentrySDK.set_attribute("deleted_attribute", "SHOULD NOT BE PRESENT")
	SentrySDK.remove_attribute("deleted_attribute")
	SentrySDK.logger.info("Test with global attributes")
