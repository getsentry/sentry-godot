extends GdUnitTestSuite
## Tests the attributes attached to automatically collected metrics.

var _metrics: Array[Dictionary] = []
var _original_scene: Node
var _scene: Node


func before_test() -> void:
	_metrics.clear()
	_original_scene = get_tree().current_scene


func after_test() -> void:
	SentrySDK.close()
	while SentrySDK.is_enabled():
		# On Web, close() is async, so we need to wait.
		await get_tree().process_frame
	get_tree().current_scene = _original_scene
	if is_instance_valid(_scene):
		_scene.free()
	_scene = null


func _init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.dsn = "http://public@127.0.0.1:1/42"
		options.debug = false
		options.auto_metrics.enable_frame_metrics = true
		options.before_send_metric = func(metric: SentryMetric) -> SentryMetric:
			var attributes: Dictionary = {}
			for key: String in ["gpu.name", "cpu.cores", "ram.gb", "res.x", "res.y", "scene"]:
				var value: Variant = metric.get_attribute(key)
				if value != null:
					attributes[key] = value
			_metrics.append({"name": metric.name, "attributes": attributes})
			return null
	)
	assert_bool(SentrySDK.is_enabled()).is_true()


func _wait_for_metrics(start_count: int = 0) -> void:
	while _metrics.size() <= start_count:
		await get_tree().process_frame


func test_automatic_metrics_include_system_attributes(_timeout: int = 10000) -> void:
	_init_sdk()
	await _wait_for_metrics()

	# On iOS with Godot 4.5, it's not safe to query memory info due to a bug: https://github.com/godotengine/godot/pull/109112
	var godot_version: Dictionary = Engine.get_version_info()
	var skip_memory_info: bool = OS.get_name() == "iOS" and godot_version.major == 4 and godot_version.minor == 5

	var cpu_cores: int = OS.get_processor_count()
	var physical_memory_bytes: int = -1
	if not skip_memory_info:
		physical_memory_bytes = OS.get_memory_info().get("physical", -1)
	var ram_gb: float = snappedf(float(physical_memory_bytes) / (1024.0 * 1024.0 * 1024.0), 0.01)
	var is_headless: bool = DisplayServer.get_name() == "headless"
	var gpu_name := ""
	var window_size: Vector2i
	if not is_headless:
		gpu_name = RenderingServer.get_video_adapter_name()
		window_size = DisplayServer.window_get_size()

	for record: Dictionary in _metrics:
		var attributes: Dictionary = record.attributes
		if cpu_cores > 0:
			assert_int(attributes["cpu.cores"]).is_equal(cpu_cores)
		else:
			assert_dict(attributes).not_contains_keys("cpu.cores")
		if physical_memory_bytes > 0:
			assert_float(attributes["ram.gb"]).is_equal(ram_gb)
		else:
			assert_dict(attributes).not_contains_keys("ram.gb")
		if is_headless:
			assert_dict(attributes).not_contains_keys("gpu.name", "res.x", "res.y")
		else:
			if not gpu_name.is_empty():
				assert_str(attributes["gpu.name"]).is_equal(gpu_name)
			else:
				assert_dict(attributes).not_contains_keys("gpu.name")
			assert_int(attributes["res.x"]).is_equal(window_size.x)
			assert_int(attributes["res.y"]).is_equal(window_size.y)


func test_manual_metrics_do_not_receive_automatic_attributes(_timeout: int = 10000) -> void:
	_init_sdk()
	await _wait_for_metrics()
	SentrySDK.metrics.gauge("test.manual", 1.0)
	assert_str(_metrics.back().name).is_equal("test.manual")
	assert_dict(_metrics.back().attributes).is_empty()


func test_scene_attribute_follows_the_current_scene(_timeout: int = 10000) -> void:
	_scene = Node.new()
	_scene.name = "CreatedInCode"
	get_tree().root.add_child(_scene)
	get_tree().current_scene = _scene
	_init_sdk()
	await _wait_for_metrics()
	for record: Dictionary in _metrics:
		assert_str(record.attributes.scene).is_equal("CreatedInCode")

	var next_scene := Node.new()
	next_scene.scene_file_path = "res://maps/MainMenu.tscn"
	get_tree().root.add_child(next_scene)
	get_tree().current_scene = next_scene
	_scene.free()
	_scene = next_scene
	var start_count: int = _metrics.size()
	await _wait_for_metrics(start_count)
	for record: Dictionary in _metrics.slice(start_count):
		assert_str(record.attributes.scene).is_equal("res://maps/MainMenu.tscn")

	start_count = _metrics.size()
	get_tree().current_scene = null
	await _wait_for_metrics(start_count)
	for record: Dictionary in _metrics.slice(start_count):
		assert_dict(record.attributes).not_contains_keys("scene")
