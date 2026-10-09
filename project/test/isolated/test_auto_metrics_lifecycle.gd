extends GdUnitTestSuite
## Tests automatic metrics lifecycle through the SDK.
## Specific collectors are tested in tests/cpp/tests/test_*_metrics_collector.cpp.

var _metric_count: int = 0


func before_test() -> void:
	_metric_count = 0


func after_test() -> void:
	await _close_sdk()
	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_RESUMED)
	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_FOCUS_IN)


func _init_sdk(enabled: bool = true) -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.dsn = "http://public@127.0.0.1:1/42"
		options.debug = false
		options.auto_metrics.enable_frame_metrics = enabled
		options.auto_metrics.frame_metrics_interval_sec = 1.0
		options.before_send_metric = func(_metric: SentryMetric) -> SentryMetric:
			_metric_count += 1
			return null
	)
	assert_bool(SentrySDK.is_enabled()).is_true()


func _close_sdk() -> void:
	var metric_count: int = _metric_count
	SentrySDK.close()
	# Web shutdown completes asynchronously after flushing, so we need to wait.
	while SentrySDK.is_enabled():
		await get_tree().process_frame
	assert_int(_metric_count).is_equal(metric_count)


func _wait_for_metrics(start_count: int = 0) -> void:
	while _metric_count <= start_count:
		await get_tree().process_frame


func _wait_real_time(duration_sec: float) -> void:
	var deadline_usec: int = Time.get_ticks_usec() + int(duration_sec * 1000000.0)
	while Time.get_ticks_usec() < deadline_usec:
		await get_tree().process_frame


func test_reinit_respects_disabled_auto_metrics(_timeout: int = 10000) -> void:
	_init_sdk()
	await _wait_for_metrics()
	await _close_sdk()
	var metric_count_before_reinit: int = _metric_count

	const enable_frame_metrics := false
	_init_sdk(enable_frame_metrics)
	await _wait_real_time(1.1)
	assert_int(_metric_count).is_equal(metric_count_before_reinit)


func test_reinit_resumes_collection(_timeout: int = 10000) -> void:
	_init_sdk()
	await _wait_for_metrics()
	var metric_count_before_reinit: int = _metric_count
	await _close_sdk()

	_init_sdk()
	await _wait_for_metrics(metric_count_before_reinit)
	assert_int(_metric_count).is_greater(metric_count_before_reinit)


func test_immediate_close_cancels_collection_startup(_timeout: int = 10000) -> void:
	_init_sdk()
	await _close_sdk()
	await _wait_real_time(1.1)
	assert_int(_metric_count).is_zero()
	_init_sdk()
	await _wait_for_metrics()


func test_application_pause_stops_collection_until_resume(_timeout: int = 10000) -> void:
	_init_sdk()
	await _wait_for_metrics()

	var metric_count_before_pause: int = _metric_count
	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_PAUSED)
	await _wait_real_time(1.1)
	assert_int(_metric_count).is_equal(metric_count_before_pause)

	var metric_count_before_resume: int = _metric_count
	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_RESUMED)
	await _wait_for_metrics(metric_count_before_resume)
	assert_int(_metric_count).is_greater(metric_count_before_resume)


func test_focus_loss_stops_collection_until_focus_returns(_timeout: int = 10000) -> void:
	_init_sdk()
	await _wait_for_metrics()

	var metric_count_before_focus_loss: int = _metric_count
	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_FOCUS_OUT)
	await _wait_real_time(1.1)
	assert_int(_metric_count).is_equal(metric_count_before_focus_loss)

	get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_FOCUS_IN)
	await _wait_for_metrics(metric_count_before_focus_loss)
	assert_int(_metric_count).is_greater(metric_count_before_focus_loss)


func test_collection_requires_both_focus_and_application_resume(_timeout: int = 10000) -> void:
	_init_sdk()
	await _wait_for_metrics()

	var restore_orders: Array[Array] = [
		[MainLoop.NOTIFICATION_APPLICATION_FOCUS_IN, MainLoop.NOTIFICATION_APPLICATION_RESUMED],
		[MainLoop.NOTIFICATION_APPLICATION_RESUMED, MainLoop.NOTIFICATION_APPLICATION_FOCUS_IN],
	]
	for notification_order: Array in restore_orders:
		var metric_count_before_suspension: int = _metric_count
		get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_FOCUS_OUT)
		get_tree().notification(MainLoop.NOTIFICATION_APPLICATION_PAUSED)
		# Repeated restoration must not clear the other suspension reason.
		get_tree().notification(notification_order[0])
		get_tree().notification(notification_order[0])
		await _wait_real_time(1.1)
		assert_int(_metric_count).is_equal(metric_count_before_suspension)

		get_tree().notification(notification_order[1])
		await _wait_for_metrics(metric_count_before_suspension)
