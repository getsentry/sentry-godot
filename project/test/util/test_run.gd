extends RefCounted
## Execute tests based on ENV variables and quit.
##
## Environment variables:
## - SENTRY_TEST_INCLUDE    ';'-separated list of paths to include in testing


func should_run() -> bool:
	_populate_env_from_android_intent()
	return OS.get_environment("SENTRY_TEST") == "1"


func execute() -> void:
	print(">>> Initializing testing")
	var scene_tree := Engine.get_main_loop() as SceneTree
	await scene_tree.process_frame

	var included_paths: PackedStringArray = _get_included_paths()
	print(" -- Tests included: ", included_paths)

	# Remove all existing nodes.
	print(" -- Cleaning scene tree...")
	for n: Node in scene_tree.root.get_children():
		n.queue_free()
	await scene_tree.process_frame

	# Add test runner node.
	print(" -- Adding test runner...")
	var test_runner: Node = load("res://test/util/test_runner.gd").new()
	scene_tree.root.add_child(test_runner)
	for path in included_paths:
		test_runner.include_tests(path)

	# Wait for completion.
	await test_runner.finished
	print(">>> Test run complete with code: ", str(test_runner.result_code))
	scene_tree.quit(test_runner.result_code)


func _get_included_paths() -> PackedStringArray:
	var include_paths: String = OS.get_environment("SENTRY_TEST_INCLUDE")
	return include_paths.split(";", false)


## Populate environment variables from Android intent extras.
func _populate_env_from_android_intent():
	if Engine.has_singleton("AndroidRuntime"):
		var android_runtime = Engine.get_singleton("AndroidRuntime")
		var activity = android_runtime.getActivity()
		if not activity:
			return
		var intent = activity.getIntent()
		if not intent:
			return
		var extras = intent.getExtras()
		if not extras:
			return
		var keys = extras.keySet().toArray()
		for i in range(keys.size()):
			var key: String = keys[i].toString()
			var value: String = extras.get(key).toString()
			if key.begins_with("SENTRY"):
				OS.set_environment(key, value)
				print("Added ", key, "=", value)
