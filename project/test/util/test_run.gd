extends RefCounted
## Execute tests based on ENV variables and quit.
##
## Environment variables:
## - SENTRY_TEST_INCLUDE    ';'-separated list of paths to include in testing


func execute() -> void:
	print(">>> Initializing testing")
	var scene_tree := Engine.get_main_loop() as SceneTree
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
