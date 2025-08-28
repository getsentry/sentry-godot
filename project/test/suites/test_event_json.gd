extends SentryTestSuite
## Deep validation of event JSON structures using JSONAssert.

# TODO: Test with unicode

signal callback_processed


func after_test() -> void:
	SentrySDK._unset_before_send()


func test_event_json_required_attributes() -> void:
	SentrySDK._set_before_send(func (event: SentryEvent) -> SentryEvent:
		var json: String = event.to_json()

		assert_json(json).describe("Required attributes exist") \
			.at("/") \
			.must_contain("event_id") \
			.must_contain("timestamp") \
			.must_contain("platform") \
			.verify()

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

		var rfc3339_regex := RegEx.create_from_string(
			"^(?:[0-9]{4}-[0-9]{2}-[0-9]{2})T(?:[0-9]{2}:[0-9]{2}:[0-9]{2})(?:\\.[0-9]+)?(?:Z|[\\+\\-][0-9]{2}:[0-9]{2})$")

		assert_json(json).describe("event.timestamp is string or number") \
			.at("/timestamp") \
			.is_one_of_types([JSONAssert.Type.NUMBER, JSONAssert.Type.STRING]) \
			.must_satisfy("must conform to RFC3339 if string", func(candidate) -> bool:
				if candidate is String:
					return rfc3339_regex.search(candidate) != null
				else:
					return true
				) \
			.verify()

		assert_json(json).describe("event.platform is non-empty string") \
			.at("/platform") \
			.is_string() \
			.is_not_empty() \
			.verify()

		assert_json(json).describe("check platform") \
			.at("/platform") \
			# TODO
			.verify()

		callback_processed.emit.call_deferred()
		return null
	)

	SentrySDK.capture_event(SentrySDK.create_event())

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_event_json_optional_attributes() -> void:
	SentrySDK._set_before_send(func (ev: SentryEvent) -> SentryEvent:
		var json: String = ev.to_json()

		assert_json(json).describe("Optional attributes are present with correct values") \
			.at("/") \
			.must_contain("level", "warning") \
			.must_contain("logger", "test-logger") \
			.must_contain("release", "my-game@1.0.0") \
			.must_contain("dist", "test-dist") \
			.must_contain("environment", "testing") \
			.verify()

		assert_json(json).describe("Message is nested under message.formatted") \
			.at("/message") \
			.is_object() \
			.must_contain("formatted", "Test event message") \
			.verify()

		assert_json(json).describe("Contains proper tags structure") \
			.at("/") \
			.must_contain("tags") \
			.is_not_empty() \
			.at("tags") \
			.must_contain("game_mode", "survival") \
			.must_contain("level_type", "procedural") \
			.verify()

		callback_processed.emit.call_deferred()
		return null
	)

	var event := SentrySDK.create_event()
	event.level = SentrySDK.LEVEL_WARNING
	event.message = "Test event message"
	event.logger = "test-logger"
	event.release = "my-game@1.0.0"
	event.dist = "test-dist"
	event.environment = "testing"
	event.set_tag("game_mode", "survival")
	event.set_tag("level_type", "procedural")
	SentrySDK.capture_event(event)

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


var expected_level: String


func test_event_json_level_attribute() -> void:
	var monitor := monitor_signals(self, false)

	SentrySDK._set_before_send(func (ev: SentryEvent) -> SentryEvent:
		assert_json(ev.to_json()).at("/level") \
			.is_string() \
			.is_not_empty() \
			.must_be(expected_level) \
			.verify()
		callback_processed.emit.call_deferred()
		return null
	)

	expected_level = "debug"
	SentrySDK.capture_message("Test level", SentrySDK.LEVEL_DEBUG)
	await assert_signal(monitor).is_emitted("callback_processed")

	expected_level = "info"
	SentrySDK.capture_message("Test level", SentrySDK.LEVEL_INFO)
	await assert_signal(monitor).is_emitted("callback_processed")

	expected_level = "warning"
	SentrySDK.capture_message("Test level", SentrySDK.LEVEL_WARNING)
	await assert_signal(monitor).is_emitted("callback_processed")

	expected_level = "error"
	SentrySDK.capture_message("Test level", SentrySDK.LEVEL_ERROR)
	await assert_signal(monitor).is_emitted("callback_processed")

	expected_level = "fatal"
	SentrySDK.capture_message("Test level", SentrySDK.LEVEL_FATAL)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_event_json_platform_contexts() -> void:
	SentrySDK._set_before_send(func (ev: SentryEvent) -> SentryEvent:
		var json: String = ev.to_json()

		assert_json(json).describe("Standard platform contexts exist") \
			.at("/contexts") \
			.is_object() \
			.is_not_empty() \
			.must_contain("app") \
			.must_contain("os") \
			.must_contain("device") \
			.must_contain("gpu") \
			.must_contain("culture") \
			.verify()

		assert_json(json).describe("App context structure") \
			.at("/contexts/app") \
			.is_object() \
			.is_not_empty() \
			.must_contain("app_name") \
			.must_contain("app_version") \
			.verify()

		assert_json(json).describe("OS context structure") \
			.at("/contexts/os") \
			.is_object() \
			.is_not_empty() \
			.must_contain("name") \
			.must_contain("version") \
			.verify()

		assert_json(json).describe("Device context structure") \
			.at("/contexts/device") \
			.is_object() \
			.is_not_empty() \
			.verify()

		# TODO: platform-specific validations

		callback_processed.emit.call_deferred()
		return null
	)

	var event := SentrySDK.create_event()
	SentrySDK.capture_event(event)

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_event_json_sdk_interface() -> void:
	SentrySDK._set_before_send(func (ev: SentryEvent) -> SentryEvent:
		var json: String = ev.to_json()

		assert_json(json).describe("SDK metadata present") \
			.at("/sdk").is_object().verify()

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

		callback_processed.emit.call_deferred()
		return null
	)

	var event := SentrySDK.create_event()
	SentrySDK.capture_event(event)

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_event_json_user_interface() -> void:
	SentrySDK._set_before_send(func (ev: SentryEvent) -> SentryEvent:
		var json: String = ev.to_json()

		assert_json(json).describe("User structure exists") \
			.at("/").must_contain("user").verify()

		assert_json(json).describe("User structure should match set attributes") \
			.at("/user") \
			.is_object()  \
			.is_not_empty() \
			.must_contain("id", "player_12345") \
			.must_contain("email", "testplayer@game.com") \
			.must_contain("username", "TestPlayer") \
			.must_contain("ip_address", "{{auto}}") \
			.verify()

		callback_processed.emit.call_deferred()
		return null
	)

	# Set user data before capturing event
	var user := SentryUser.new()
	user.id = "player_12345"
	user.email = "testplayer@game.com"
	user.username = "TestPlayer"
	user.ip_address = "{{auto}}"
	SentrySDK.set_user(user)

	var event := SentrySDK.create_event()
	SentrySDK.capture_event(event)

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_event_json_user_interface_with_empty_user() -> void:
	SentrySDK._set_before_send(func (ev: SentryEvent) -> SentryEvent:
		var json: String = ev.to_json()

		assert_json(json).describe("User structure contains only the automatic ID") \
			.either() \
				.must_not_contain("/user") \
			.or_else() \
				.at("/user") \
				.is_object()  \
				.must_not_contain("email") \
				.must_not_contain("username") \
				.must_not_contain("ip_address") \
			.end() \
			.verify()

		callback_processed.emit.call_deferred()
		return null
	)

	SentrySDK.remove_user()
	SentrySDK.capture_event(SentrySDK.create_event())

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_event_json_message_interface() -> void:
	SentrySDK._set_before_send(func (ev: SentryEvent) -> SentryEvent:
		var json: String = ev.to_json()

		assert_json(json).describe("Basic attributes exist") \
			.at("/") \
			.must_contain("event_id") \
			.must_contain("timestamp") \
			.must_contain("platform") \
			.verify()

		assert_json(json).describe("Event has correct level") \
			.at("/").must_contain("level", "warning").verify()

		assert_json(json).describe("Message interface structure") \
			.at("/message") \
			.is_object() \
			.is_not_empty() \
			.must_contain("formatted", "Nobody expects the Spanish Inquisition!") \
			.verify()

		callback_processed.emit.call_deferred()
		return null
	)

	SentrySDK.capture_message("Nobody expects the Spanish Inquisition!", SentrySDK.LEVEL_WARNING)

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")
