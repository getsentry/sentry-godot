extends SentryTestSuite

signal log_processed(entry: SentryLog)

func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.experimental.enable_logs = true
		options.experimental.before_send_log = _before_send_log
	)


func _before_send_log(entry: SentryLog) -> SentryLog:
	log_processed.emit(entry)
	return null


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


func test_strucutured_logs_with_utf8() -> void:
	log_processed.connect(func(entry: SentryLog):
		assert_str(entry.body).is_equal("Hello 世界! 👋")
		assert_str(entry.get_attribute("sentry.message.template")).is_equal("Hello %s! 👋")
		assert_str(entry.get_attribute("sentry.message.parameter.0")).is_equal("世界")
	, CONNECT_ONE_SHOT)
	SentrySDK.logger.info("Hello %s! 👋", ["世界"], {
		"world": "世界"
	})
