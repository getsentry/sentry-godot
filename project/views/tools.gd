extends VBoxContainer

const MOBILE_TESTS_FILE := "res://test/mobile_tests.gd"


func _ready() -> void:
	%RunTestsButton.visible = DirAccess.dir_exists_absolute("res://test/suites/")


func _on_run_tests_button_pressed() -> void:
	var test_runner = load("res://test/util/test_runner.gd").new()
	test_runner.include_tests("res://test/suites/")
	add_child(test_runner)
	await test_runner.finished
	DemoOutput.print_info("Tests finished with code: %d" % test_runner.result_code)
	test_runner.queue_free()


func _on_test_diverse_context_button_pressed() -> void:
	var context := {
		"null": null,
		"bool": true,
		"int": 42,
		"float": 42.42,
		"string": "hello, world!",
		"Vector2": Vector2(123.45, 67.89),
		"Vector2i": Vector2i(123, 45),
		"Rect2": Rect2(123.45, 67.89, 98.76, 54.32),
		"Rect2i": Rect2i(12, 34, 56, 78),
		"Vector3": Vector3(12.34, 56.78, 90.12),
		"Vector3i": Vector3i(12, 34, 56),
		"Transform2D": Transform2D().translated(Vector2(12.34, 56.78)),
		"Vector4": Vector4(12.34, 56.78, 90.12, 34.56),
		"Vector4i": Vector4i(12, 34, 56, 78),
		"Plane": Plane(Vector3(1, 2, 3), 4),
		"Quaternion": Quaternion(Vector3(0, 1, 0), 4),
		"AABB": AABB(Vector3(1, 2, 3), Vector3(4, 5, 6)),
		"Basis": Basis(Vector3(1, 2, 3), Vector3(4, 5, 6), Vector3(7, 8, 9)),
		"Transform3D": Transform3D().translated(Vector3(12.34, 56.78, 90.12)),
		"Projection": Projection(Vector4(73.21, 19.47, 85.63, 73.02), Vector4(41.92, 67.38, 22.14, 59.81), Vector4(93.76, 15.49, 38.72, 84.25), Vector4(26.58, 71.93, 47.16, 62.84)),
		"Color": Color(12.34, 56.78, 90.12, 34.56),
		"StringName": StringName("hello, world!"),
		"NodePath": NodePath("/root"),
		"RID": RID(),
		"Object": self,
		"Callable": _on_test_diverse_context_button_pressed,
		"Signal": get_tree().process_frame,
		"Dictionary": {"key1": "value1", "key2": 42, "key3": self},
		"Array": [1, self, {"hello": "world"}],
		"PackedByteArray": PackedByteArray([1, 2, 3, 4, 5]),
		"PackedInt32Array": PackedInt32Array([1, 2, 3, 4, 5]),
		"PackedInt64Array": PackedInt64Array([1, 2, 3, 4, 5]),
		"PackedFloat32Array": PackedFloat32Array([1.23, 4.56, 7.89]),
		"PackedFloat64Array": PackedFloat64Array([1.23, 4.56, 7.89]),
		"PackedStringArray": PackedStringArray(["hello", "world"]),
		"PackedVector2Array": PackedVector2Array([Vector2(1, 2), Vector2(3, 4)]),
		"PackedVector3Array": PackedVector3Array([Vector3(1, 2, 3), Vector3(4, 5, 6)]),
		"PackedColorArray": PackedColorArray([Color(1, 2, 3, 4), Color(5, 6, 7, 8)]),
		"PackedVector4Array": PackedVector4Array([Vector4(1, 2, 3, 4), Vector4(5, 6, 7, 8)]),
	}
	SentrySDK.set_context("diverse_context", context)
	DemoOutput.print_info("Added context with diverse values.")
	SentrySDK.capture_message("Test diverse context")
	if OS.get_name() == "Android":
		# Check if we should run gdUnit4 tests or the simple Android tests
		var run_gdunit_tests = OS.get_environment("SENTRY_RUN_GDUNIT_TESTS")
		if run_gdunit_tests == "true":
			# Load and run the mobile test runner scene
			var mobile_test_scene = load("res://mobile_test.tscn").instantiate()
			get_tree().root.add_child(mobile_test_scene)
			# Hide the current mobile demo UI
			visible = false
		else:
			# Run the original simple Android tests
			var tests = load("res://test/test_on_android.gd").new()
			tests.run_tests()
