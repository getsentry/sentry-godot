extends RefCounted

var num_passed: int = 0
var num_failed: int = 0


func run_tests() -> void:
	print("--------------------------------------------------------------------")
	print("Running Android tests:")

	test_capture_message()
	test_capture_event()

	SentrySDK._unset_before_send()

	print("Passed: ", num_passed, "  Failed: ", num_failed)
	print("--------------------------------------------------------------------")


func test_capture_message() -> void:
	SentrySDK._set_before_send(
		func(ev: SentryEvent) -> SentryEvent:
			assert_true(ev.message == "test-message", "capture_message/event.message")
			assert_equal(ev.level, SentrySDK.LEVEL_DEBUG, "capture_message/event.level")
			return null
	)

	SentrySDK.capture_message("test-message", SentrySDK.LEVEL_DEBUG)


var created_id
func test_capture_event() -> void:
	SentrySDK._set_before_send(
		func(event: SentryEvent) -> SentryEvent:
			assert_equal(event.message, "integrity-check", "capture_event/event.message")
			assert_equal(event.level, SentrySDK.LEVEL_DEBUG, "capture_event/event.level")
			assert_equal(event.logger, "custom-logger", "capture_event/event.logger")
			assert_equal(event.release, "custom-release", "capture_event/event.release")
			assert_equal(event.dist, "custom-dist", "capture_event/event.dist")
			assert_equal(event.environment, "custom-environment", "capture_event/event.environment")
			assert_equal(event.get_tag("custom-tag"), "custom-tag-value", "capture_event/event.tag")
			assert_equal(event.id, created_id, "capture_event/id")
			assert_false(event.is_crash(), "capture_event/is_crash is false")
			return null # discard event
	)

	var event := SentrySDK.create_event()
	event.message = "integrity-check"
	event.level = SentrySDK.LEVEL_DEBUG
	event.logger = "custom-logger"
	event.release = "custom-release"
	event.dist = "custom-dist"
	event.environment = "custom-environment"
	event.set_tag("custom-tag", "custom-tag-value")
	created_id = event.id

	var captured_id := SentrySDK.capture_event(event)
	assert_not_equal(captured_id, SentrySDK.get_last_event_id(), "capture_event/last_id_check")


# --------------------------------------------------------------------------------------------------

func assert_true(predicate: bool, name: String) -> void:
	if predicate:
		_passed(name)
	else:
		_failed(name)


func assert_false(predicate: bool, name: String) -> void:
	if predicate == false:
		_passed(name)
	else:
		_failed(name)


func assert_equal(a, b, name: String) -> void:
	if a == b:
		_passed(name)
	else:
		_failed(name)
	#print("  a: ", a, " b: ", b)


func assert_not_equal(a, b, name: String) -> void:
	if a != b:
		_passed(name)
	else:
		_failed(name)
	#print("  a: ", a, " b: ", b)


func _passed(name: String):
	print("PASSED: ", name)
	num_passed += 1


func _failed(name: String):
	print("FAILED: ", name)
	num_failed += 1
