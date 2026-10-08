extends GdUnitTestSuite

var _metrics: Array[Dictionary] = []


func before_test() -> void:
	await _close_sdk()
	_metrics.clear()


func after_test() -> void:
	await _close_sdk()
	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_RESUMED)


func _init_sdk(enabled: bool = true, interval_sec: float = 0.05) -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.dsn = "http://public@127.0.0.1:1/42"
		options.debug = false
		options.auto_metrics.enable_frame_metrics = enabled
		options.auto_metrics.frame_metrics_interval_sec = interval_sec
		options.before_send_metric = _before_send_metric
	)
	assert_bool(SentrySDK.is_enabled()).is_true()


func _close_sdk() -> void:
	var metric_count: int = _metrics.size()
	SentrySDK.close()
	# Web shutdown completes asynchronously after flushing, so we need to wait.
	while SentrySDK.is_enabled():
		await get_tree().process_frame
	assert_int(_metrics.size()).is_equal(metric_count)


func _before_send_metric(metric: SentryMetric) -> SentryMetric:
	_metrics.append({
		"name": metric.name,
		"type": metric.type,
		"unit": metric.unit,
		"frame": Engine.get_process_frames(),
		"emitted_at_usec": Time.get_ticks_usec(),
	})
	return null


## Returns metric names in the order received, from start_index onward.
func _metric_names(start_index: int = 0) -> Array[String]:
	var names: Array[String] = []
	for metric: Dictionary in _metrics.slice(start_index):
		names.append(metric.name)
	return names


## Checks that records from start_index onward include frame time and FPS with
## their expected metric types and units.
func _assert_frame_metrics(start_index: int = 0) -> void:
	assert_array(_metric_names(start_index)).contains("game.perf.frame_time", "game.perf.fps")
	for metric: Dictionary in _metrics.slice(start_index):
		match metric.name:
			"game.perf.frame_time":
				assert_int(metric.type).is_equal(SentryMetric.METRIC_DISTRIBUTION)
				assert_str(metric.unit).is_equal("millisecond")
			"game.perf.fps":
				assert_int(metric.type).is_equal(SentryMetric.METRIC_GAUGE)
				assert_str(metric.unit).is_empty()


## Waits for repeated reports and checks that each reporting frame has one metric pair.
## Frames that do not reach the reporting deadline may emit nothing.
func _assert_one_collecting_stream() -> void:
	var start_index: int = _metrics.size()
	while _metric_names(start_index).count("game.perf.fps") < 8:
		await get_tree().process_frame
	var metrics_by_frame: Dictionary = {}
	for metric: Dictionary in _metrics.slice(start_index):
		if not metrics_by_frame.has(metric.frame):
			metrics_by_frame[metric.frame] = []
		metrics_by_frame[metric.frame].append(metric.name)
	assert_int(metrics_by_frame.size()).is_greater_equal(8)
	for names: Array in metrics_by_frame.values():
		assert_array(names).contains_exactly_in_any_order("game.perf.frame_time", "game.perf.fps")


func test_enabled_frame_metrics_are_emitted() -> void:
	_init_sdk()
	await get_tree().create_timer(0.2).timeout
	_assert_frame_metrics()


func test_reinit_respects_disabled_auto_metrics() -> void:
	_init_sdk()
	await get_tree().create_timer(0.2).timeout
	_assert_frame_metrics()
	var metric_count: int = _metrics.size()
	await _close_sdk()

	_init_sdk(false)
	await get_tree().create_timer(0.2).timeout
	assert_int(_metrics.size()).is_equal(metric_count)

	# Sanity check
	SentrySDK.metrics.gauge("test.manual", 1.0)
	assert_array(_metric_names(metric_count)).contains_exactly("test.manual")


func test_reinit_resumes_collection_without_duplicates() -> void:
	# A short interval helps detect overlapping collectors.
	_init_sdk(true, 0.000001)
	await _assert_one_collecting_stream()
	await _close_sdk()

	_init_sdk(true, 0.000001)
	await _assert_one_collecting_stream()


func test_immediate_reinit_does_not_duplicate_metrics() -> void:
	_init_sdk(true, 0.000001)
	await _close_sdk()
	assert_array(_metrics).is_empty()
	_init_sdk(true, 0.000001)
	await _assert_one_collecting_stream()


func test_application_pause_stops_frame_metrics_until_resume() -> void:
	_init_sdk(true, 0.000001)
	await _assert_one_collecting_stream()

	var metric_count_before_pause: int = _metrics.size()
	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_PAUSED)
	await get_tree().create_timer(0.2).timeout
	assert_int(_metrics.size()).is_equal(metric_count_before_pause)
	assert_bool(SentrySDK.is_enabled()).is_true()
	SentrySDK.metrics.gauge("test.manual", 1.0)
	assert_array(_metric_names(metric_count_before_pause)).contains_exactly("test.manual")

	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_RESUMED)
	await _assert_one_collecting_stream()


func test_application_resume_starts_a_fresh_reporting_window() -> void:
	_init_sdk()
	while not _metric_names().has("game.perf.fps"):
		await get_tree().process_frame
	await get_tree().process_frame
	var metric_count_before_pause: int = _metrics.size()

	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_PAUSED)
	await get_tree().create_timer(0.2).timeout
	assert_int(_metrics.size()).is_equal(metric_count_before_pause)

	var resumed_at_usec: int = Time.get_ticks_usec()
	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_RESUMED)
	while not _metric_names(metric_count_before_pause).has("game.perf.fps"):
		await get_tree().process_frame
	_assert_frame_metrics(metric_count_before_pause)
	for metric: Dictionary in _metrics.slice(metric_count_before_pause):
		# 50000 microseconds is the default reporting interval used by _init_sdk(): 0.05 seconds.
		assert_int(metric.emitted_at_usec - resumed_at_usec).is_greater_equal(50000)
