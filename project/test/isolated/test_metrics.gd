extends SentryTestSuite

signal metric_processed(metric: SentryMetric)

var _discard_metric: bool = false


func before(_do_skip := OS.get_name() in ["macOS", "iOS"], _skip_reason := "Metrics are not supported on Apple platforms") -> void:
	super()


func after() -> void:
	_discard_metric = false
	super()


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.experimental.enable_metrics = true
		options.experimental.before_send_metric = _before_send_metric
	)


func _before_send_metric(metric: SentryMetric) -> SentryMetric:
	if _discard_metric:
		return null
	metric_processed.emit(metric)
	return metric


func test_count() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.name).is_equal("button_click")
		assert_int(metric.type).is_equal(SentryMetric.METRIC_COUNTER)
		assert_float(metric.value).is_equal(1.0)
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.count("button_click")


func test_count_with_custom_value() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.name).is_equal("items_collected")
		assert_float(metric.value).is_equal(5.0)
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.count("items_collected", 5)


func test_gauge() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.name).is_equal("cpu_temperature")
		assert_int(metric.type).is_equal(SentryMetric.METRIC_GAUGE)
		assert_float(metric.value).is_equal_approx(72.5, 0.001)
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.gauge("cpu_temperature", 72.5)


func test_gauge_with_unit() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.name).is_equal("memory_usage")
		assert_str(metric.unit).is_equal("byte")
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.gauge("memory_usage", 1024.0, "byte")


func test_distribution() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.name).is_equal("response_time")
		assert_int(metric.type).is_equal(SentryMetric.METRIC_DISTRIBUTION)
		assert_float(metric.value).is_equal_approx(150.0, 0.001)
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.distribution("response_time", 150.0)


func test_gauge_with_sentry_unit() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.unit).is_equal("byte")
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.gauge("memory_usage", 1024.0, SentryUnit.byte)


func test_distribution_with_sentry_unit() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.unit).is_equal("millisecond")
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.distribution("level_load_time", 150.0, SentryUnit.millisecond)


func test_distribution_with_unit() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.name).is_equal("request_size")
		assert_str(metric.unit).is_equal("kilobyte")
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.distribution("request_size", 256.0, "kilobyte")


func test_count_with_attributes() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.get_attribute("level")).is_equal("forest")
		assert_int(metric.get_attribute("enemy_id")).is_equal(42)
		assert_float(metric.get_attribute("health")).is_equal_approx(10.5, 0.001)
		assert_bool(metric.get_attribute("elite")).is_equal(false)
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.count("enemy_defeated", 1, {
		"level": "forest",
		"enemy_id": 42,
		"health": 10.5,
		"elite": false
	})


func test_metric_attribute_methods() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		metric.add_attributes({
			"hello": "world",
			"meaning": 42
		})
		assert_str(metric.get_attribute("hello")).is_equal("world")
		assert_int(metric.get_attribute("meaning")).is_equal(42)

		metric.set_attribute("test", true)
		assert_bool(metric.get_attribute("test")).is_true()

		metric.set_attribute("PI", 3.14)
		assert_float(metric.get_attribute("PI")).is_equal_approx(3.14, 0.001)

		metric.remove_attribute("hello")
		assert_that(metric.get_attribute("hello")).is_null()

		metric.remove_attribute("meaning")
		assert_that(metric.get_attribute("meaning")).is_null()
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.count("test_metric")


func test_metric_set_name() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.name).is_equal("original_name")
		metric.name = "modified_name"
		assert_str(metric.name).is_equal("modified_name")
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.count("original_name")


func test_metric_set_value() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_float(metric.value).is_equal_approx(100.0, 0.001)
		metric.value = 999.0
		assert_float(metric.value).is_equal_approx(999.0, 0.001)
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.gauge("test_gauge", 100.0)


func test_metric_with_utf8() -> void:
	metric_processed.connect(func(metric: SentryMetric):
		assert_str(metric.name).is_equal("Hello 世界! 👋")
		assert_str(metric.unit).is_equal("ポイント")
		assert_str(metric.get_attribute("world")).is_equal("世界")
	, CONNECT_ONE_SHOT)
	SentrySDK.metrics.gauge("Hello 世界! 👋", 100.0, "ポイント", {
		"world": "世界"
	})


func test_before_send_metric_discard() -> void:
	_discard_metric = true
	var monitor := monitor_signals(self, false)
	SentrySDK.metrics.count("discarded_metric")
	await assert_signal(monitor).is_not_emitted("metric_processed")
	_discard_metric = false
