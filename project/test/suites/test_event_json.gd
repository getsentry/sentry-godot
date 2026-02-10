extends SentryTestSuite
## Deep validation of event JSON structures using JSONAssert.


func test_event_json_has_required_attributes() -> void:
	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Required attributes exist") \
		.at("/") \
		.must_contain("event_id") \
		.must_contain("timestamp") \
		.must_contain("platform") \
		.verify()


func test_event_json_has_proper_event_id() -> void:
	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("event_id is 32-char hex") \
		.at("/event_id") \
		.is_string() \
		.either() \
			.has_size(32) \
		.or_else() \
			.has_size(36) \
		.end() \
		.must_satisfy("valid UUID", func(candidate) -> bool:
			for chr in candidate:
				if not chr in "0123456789abcdef-":
					return false
			return true) \
		.verify()


func test_event_json_has_proper_timestamp() -> void:
	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	var rfc3339_regex := RegEx.create_from_string(
		"^(?:[0-9]{4}-[0-9]{2}-[0-9]{2})T(?:[0-9]{2}:[0-9]{2}:[0-9]{2})(?:\\.[0-9]+)?(?:Z|[\\+\\-][0-9]{2}:[0-9]{2})$")

	assert_json(json).describe("event.timestamp is RFC3339-formatted string or number") \
		.at("/timestamp") \
		.either() \
			.is_number() \
		.or_else() \
			.is_string() \
			.must_match_regex(rfc3339_regex) \
		.end() \
		.verify()


func test_event_json_has_proper_platform() -> void:
	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("event.platform is non-empty string") \
		.at("/platform") \
		.is_string() \
		.is_not_empty() \
		.verify()

	assert_json(json).describe("check platform") \
		.at("/platform") \
		.must_satisfy("event.platform value",
			func(candidate) -> bool:
				if candidate is not String:
					return false
				match OS.get_name():
					"Windows", "Linux":
						return candidate == "native"
					"macOS", "iOS":
						return candidate == "cocoa"
					"Android":
						return candidate == "java"
					"Web":
						return candidate == "javascript"
					_:
						return false
				) \
		.verify()


func test_event_json_optional_attributes() -> void:
	var event := SentrySDK.create_event()
	event.level = SentrySDK.LEVEL_WARNING
	event.logger = "test-logger"
	event.release = "my-game@1.0.0"
	event.dist = "test-dist"
	event.environment = "testing"

	var json: String = await capture_event_and_get_json(event)

	assert_json(json).describe("Optional attributes are present with correct values") \
		.at("/") \
		.must_contain("level", "warning") \
		.must_contain("logger", "test-logger") \
		.must_contain("release", "my-game@1.0.0") \
		.must_contain("dist", "test-dist") \
		.must_contain("environment", "testing") \
		.verify()


func test_event_json_message_interface() -> void:
	var event := SentrySDK.create_event()
	event.message = "Test event message"

	var json: String = await capture_event_and_get_json(event)

	assert_json(json).describe("Message interface structure") \
		.at("/message") \
		.either() \
			.is_object() \
			.must_contain("formatted", "Test event message") \
		.or_else() \
			.must_be("Test event message") \
		.end() \
		.verify()


func test_event_json_with_capture_message() -> void:
	SentrySDK.capture_message("Nobody expects the Spanish Inquisition!", SentrySDK.LEVEL_WARNING)

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("Message interface structure") \
		.at("/") \
		.either() \
			.must_contain("message/formatted", "Nobody expects the Spanish Inquisition!") \
		.or_else() \
			.must_contain("message", "Nobody expects the Spanish Inquisition!") \
		.end() \
		.verify()

	assert_json(json).describe("Event has correct level") \
		.at("/").must_contain("level", "warning").verify()


func test_event_json_with_event_tags() -> void:
	var event := SentrySDK.create_event()
	event.set_tag("game_mode", "survival")
	event.set_tag("level_type", "procedural")

	var json: String = await capture_event_and_get_json(event)

	assert_json(json).describe("Contains proper tags structure") \
		.at("/") \
		.must_contain("tags") \
		.is_not_empty() \
		.at("tags") \
		.must_contain("game_mode", "survival") \
		.must_contain("level_type", "procedural") \
		.verify()


func test_event_json_with_global_tags() -> void:
	SentrySDK.set_tag("game_mode", "survival")
	SentrySDK.set_tag("level_type", "procedural")

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Contains proper tags structure") \
		.at("/") \
		.must_contain("tags") \
		.is_not_empty() \
		.at("tags") \
		.must_contain("game_mode", "survival") \
		.must_contain("level_type", "procedural") \
		.verify()


func test_event_json_attributes_with_utf8_encoding() -> void:
	var event := SentrySDK.create_event()
	event.message = "Hello 世界! 👋"
	event.logger = "Hello 世界! 👋"
	event.release = "Hello 世界! 👋"
	event.dist = "Hello 世界! 👋"
	event.environment = "Hello 世界! 👋"
	event.set_tag("tag1", "Hello 世界! 👋")
	SentrySDK.set_tag("global-tag1", "Hello 世界! 👋")
	SentrySDK.set_context("greetings", {"Hello, World! 👋": "Hello 世界! 👋"})
	SentrySDK.add_breadcrumb(SentryBreadcrumb.create("Hello 世界! 👋"))

	var json: String = await capture_event_and_get_json(event)

	assert_json(json).describe("Attributes preserve UTF-8 data") \
		.at("/") \
		.must_contain("logger", "Hello 世界! 👋") \
		.must_contain("release", "Hello 世界! 👋") \
		.must_contain("dist", "Hello 世界! 👋") \
		.must_contain("environment", "Hello 世界! 👋") \
		.verify()

	assert_json(json).describe("Message preserves UTF-8 data") \
		.at("/") \
		.either() \
			.must_contain("message/formatted", "Hello 世界! 👋") \
		.or_else() \
			.must_contain("message", "Hello 世界! 👋") \
		.end() \
		.verify()

	assert_json(json).describe("Tags preserve UTF-8 data") \
		.at("/tags") \
		.must_contain("tag1", "Hello 世界! 👋") \
		.must_contain("global-tag1", "Hello 世界! 👋") \
		.verify()

	assert_json(json).describe("Contexts preserve UTF-8 data") \
		.at("/contexts") \
		.must_contain("greetings", {"Hello, World! 👋": "Hello 世界! 👋"}) \
		.verify()


func test_capture_message_with_utf8() -> void:
	SentrySDK.capture_message("Hello 世界! 👋")

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("Message preserves UTF-8 data") \
		.at("/") \
		.either() \
			.must_contain("message/formatted", "Hello 世界! 👋") \
		.or_else() \
			.must_contain("message", "Hello 世界! 👋") \
		.end() \
		.verify()


func test_event_json_level_attribute() -> void:
	var expected_levels: PackedStringArray

	expected_levels.append("debug")
	SentrySDK.capture_message("Test level", SentrySDK.LEVEL_DEBUG)
	await assert_signal(self).is_emitted("event_captured")

	expected_levels.append("info")
	SentrySDK.capture_message("Test level", SentrySDK.LEVEL_INFO)
	await assert_signal(self).is_emitted("event_captured")

	expected_levels.append("warning")
	SentrySDK.capture_message("Test level", SentrySDK.LEVEL_WARNING)
	await assert_signal(self).is_emitted("event_captured")

	expected_levels.append("error")
	SentrySDK.capture_message("Test level", SentrySDK.LEVEL_ERROR)
	await assert_signal(self).is_emitted("event_captured")

	expected_levels.append("fatal")
	SentrySDK.capture_message("Test level", SentrySDK.LEVEL_FATAL)
	await assert_signal(self).is_emitted("event_captured")

	for i in captured_events.size():
		var json: String = captured_events[i]
		var expected_level: String = expected_levels[i]
		assert_json(json).describe("Verify level of event #" + str(i)) \
			.at("/level") \
			.is_string() \
			.is_not_empty() \
			.must_be(expected_level) \
			.verify()


# NOTE: JS SDK adds SDK info after beforeSend; therefore, skip this test in Web builds.
func test_event_json_sdk_interface(_do_skip = OS.get_name() == "Web") -> void:
	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("SDK metadata present") \
		.at("/sdk").is_object().is_not_empty().verify()

	assert_json(json).describe("Has SDK name that follows sentry.*.godot pattern") \
		.at("/sdk/name") \
		.is_string() \
		.is_not_empty() \
		.must_begin_with("sentry.") \
		.must_end_with(".godot") \
		.verify()

	assert_json(json).describe("Has SDK version with semantic versioning format") \
		.at("/sdk/version") \
		.is_string() \
		.is_not_empty() \
		.must_satisfy("valid version string", func(candidate) -> bool:
			if not candidate is String:
				return false
			for part in candidate.split("."):
				if not part.is_valid_int() or part.to_int() < 0:
					return false
			return true
			) \
		.verify()
