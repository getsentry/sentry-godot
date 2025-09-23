extends RefCounted

var num_passed: int = 0
var num_failed: int = 0


func run_tests() -> void:
	print("--------------------------------------------------------------------")
	print("Running Android tests:")

	var prev: Callable = SentrySDK._get_before_send()

	test_capture_message()
	test_capture_event()

	SentrySDK._set_before_send(prev)

	print("Passed: ", num_passed, "  Failed: ", num_failed)
	print("--------------------------------------------------------------------")


func test_capture_message() -> void:
	SentrySDK._set_before_send(
		func(ev: SentryEvent) -> SentryEvent:
			assert_true(ev.message == "test-message", "capture_message(): event.message retains its value")
			assert_equal(ev.level, SentrySDK.LEVEL_DEBUG, "capture_message(): event.level retains its value")
			return null
	)

	SentrySDK.capture_message("test-message", SentrySDK.LEVEL_DEBUG)


var created_id: String
var timestamp: SentryTimestamp
func test_capture_event() -> void:
	SentrySDK._set_before_send(
		func(event: SentryEvent) -> SentryEvent:
			assert_equal(event.message, "integrity-check", "capture_event(): event.message retains its value")
			assert_equal(event.level, SentrySDK.LEVEL_DEBUG, "capture_event(): event.level retains its value")
			assert_equal(event.logger, "custom-logger", "capture_event(): event.logger retains its value")
			assert_equal(event.release, "custom-release", "capture_event(): event.release retains its value")
			assert_equal(event.dist, "custom-dist", "capture_event(): event.dist retains its value")
			assert_equal(event.environment, "custom-environment", "capture_event(): event.environment retains its value")
			assert_equal(event.get_tag("custom-tag"), "custom-tag-value", "capture_event(): event retains custom tag value")
			assert_equal(event.id, created_id, "capture_event(): event.id retains its value")
			assert_false(event.is_crash(), "capture_event(): event.is_crash() should be false")
			assert_not_null(event.timestamp, "capture_event(): event.timestamp should not be null")
			assert_true(event.timestamp.equals(timestamp), "capture_event(): event.timestamp retains its value")

			if OS.get_name() == "Android":
				assert_equal(event.platform, "java", "capture_event(): event.platfrom returns 'java' on Android")

			return null # discard event
	)

	var ev := SentrySDK.create_event()
	ev.message = "integrity-check"
	ev.level = SentrySDK.LEVEL_DEBUG
	ev.logger = "custom-logger"
	ev.release = "custom-release"
	ev.dist = "custom-dist"
	ev.environment = "custom-environment"
	ev.set_tag("custom-tag", "custom-tag-value")

	timestamp = SentryTimestamp.parse_rfc3339("2025-06-02T14:45:01.123Z")
	ev.timestamp = timestamp

	created_id = ev.id

	SentrySDK.capture_event(ev)


# --------------------------------------------------------------------------------------------------

func assert_true(predicate: bool, name: String) -> void:
	if predicate:
		_passed(name)
	else:
		_failed(name)
		print("  - predicate expected to be true")


func assert_false(predicate: bool, name: String) -> void:
	if predicate == false:
		_passed(name)
	else:
		_failed(name)
		print("  - predicate expected to be false")



func assert_equal(a, b, name: String) -> void:
	if a == b:
		_passed(name)
	else:
		_failed(name)
		print("  - ", a, " expected to be equal to ", b)


func assert_not_equal(a, b, name: String) -> void:
	if a != b:
		_passed(name)
	else:
		_failed(name)
		print("  - ", a, " expected not to be equal to ", b)


func assert_not_null(obj: Object, name: String) -> void:
	if obj != null:
		_passed(name)
	else:
		_failed(name)
		print("  - object is expected to be not null")


func _passed(name: String):
	print("PASSED: ", name)
	num_passed += 1


func _failed(name: String):
	print("FAILED: ", name)
	num_failed += 1
