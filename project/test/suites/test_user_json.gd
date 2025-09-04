extends SentryTestSuite
## User interface integration tests using JSON validation.

var _old_user: SentryUser


func before_test() -> void:
	super()
	# Save user
	_old_user = SentrySDK.get_user()


func after_test() -> void:
	super()
	# Restore user
	if SentrySDK.get_user() != _old_user:
		SentrySDK.set_user(_old_user)


func test_full_user_data_in_captured_events() -> void:
	var user := SentryUser.new()
	user.id = "player_12345"
	user.email = "testplayer@game.com"
	user.username = "TestPlayer"
	user.ip_address = "{{auto}}"
	SentrySDK.set_user(user)

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("User structure should match set attributes") \
		.at("/user") \
		.is_object()  \
		.is_not_empty() \
		.must_contain("id", "player_12345") \
		.must_contain("email", "testplayer@game.com") \
		.must_contain("username", "TestPlayer") \
		.must_contain("ip_address", "{{auto}}") \
		.verify()


func test_user_data_with_utf8_encoding() -> void:
	var user := SentryUser.new()
	user.id = "さくら🌸"
	user.username = "さくら🌸"
	user.email = "桜🌸@例え.テスト"
	SentrySDK.set_user(user)

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("User structure should preserve UTF-8 encoding") \
		.at("/user") \
		.is_object()  \
		.is_not_empty() \
		.must_contain("id", "さくら🌸") \
		.must_contain("username", "さくら🌸") \
		.must_contain("email", "桜🌸@例え.テスト") \
		.verify()


func test_user_data_after_remove() -> void:
	# Set full user data first, and then remove it.
	var user := SentryUser.new()
	user.id = "player_12345"
	user.email = "testplayer@game.com"
	user.username = "TestPlayer"
	user.ip_address = "{{auto}}"
	SentrySDK.set_user(user)
	SentrySDK.remove_user()

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

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


func test_user_transitions_and_minimal_data() -> void:
	# Test user with only ID
	var user_id_only := SentryUser.new()
	user_id_only.id = "id_only_user"
	SentrySDK.set_user(user_id_only)

	var json_with_id: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_with_id).describe("User with only ID should have clean structure") \
		.at("/user") \
		.is_object() \
		.is_not_empty() \
		.must_contain("id", "id_only_user") \
		.must_not_contain("email") \
		.must_not_contain("username") \
		.must_not_contain("ip_address") \
		.verify()

	# Test transition to full user
	var user1 := SentryUser.new()
	user1.id = "player_001"
	user1.email = "player1@game.com"
	user1.username = "Player1"
	user1.ip_address = "192.168.1.1"
	SentrySDK.set_user(user1)

	var json_with_full: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_with_full).describe("Complete user should appear correctly") \
		.at("/user") \
		.is_object() \
		.is_not_empty() \
		.must_contain("id", "player_001") \
		.must_contain("email", "player1@game.com") \
		.must_contain("username", "Player1") \
		.must_contain("ip_address", "192.168.1.1") \
		.verify()

	# Test transition to user with only email
	var user_email_only := SentryUser.new()
	user_email_only.email = "email.only@game.com"
	SentrySDK.set_user(user_email_only)

	var json_with_email: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_with_email).describe("User with only email should have clean structure") \
		.at("/user") \
		.is_object() \
		.is_not_empty() \
		.must_contain("email", "email.only@game.com") \
		.must_not_contain("id") \
		.must_not_contain("username") \
		.must_not_contain("ip_address") \
		.verify()

	# Test transition to user with only username
	var user_username_only := SentryUser.new()
	user_username_only.username = "UsernameOnly"
	SentrySDK.set_user(user_username_only)

	var json_with_username: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json_with_username).describe("User with only username should have clean structure") \
		.at("/user") \
		.is_object() \
		.is_not_empty() \
		.must_contain("username", "UsernameOnly") \
		.must_not_contain("id") \
		.must_not_contain("email") \
		.must_not_contain("ip_address") \
		.verify()


func test_user_persistence_across_events() -> void:
	# Set user once
	var user := SentryUser.new()
	user.id = "session_player"
	user.email = "session@game.com"
	user.username = "SessionPlayer"
	SentrySDK.set_user(user)

	# Test persistence across different event types
	SentrySDK.capture_message("Test message")
	var message_json: String = await wait_for_captured_event_json()

	var custom_event := SentrySDK.create_event()
	custom_event.message = "Test custom event"
	var custom_json: String = await capture_event_and_get_json(custom_event)

	push_error("Test error")
	var error_json: String = await wait_for_captured_event_json()

	# Verify user data persists and is identical across event types
	var expected_user := {
		"id": "session_player",
		"email": "session@game.com",
		"username": "SessionPlayer"
	}

	assert_json(message_json).describe("Message event contains consistent user data") \
		.at("/user").must_be(expected_user).verify()

	assert_json(custom_json).describe("Custom event contains consistent user data") \
		.at("/user").must_be(expected_user).verify()

	assert_json(error_json).describe("Error event contains consistent user data") \
		.at("/user").must_be(expected_user).verify()
