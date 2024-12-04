class_name TestSentryUser
extends GdUnitTestSuite

## Make a SentryUser instance with pre-filled test data.
func _make_test_user() -> SentryUser:
	var user := SentryUser.new()
	user.id = "custom_id"
	user.email = "bob@example.com"
	user.username = "bob"
	user.ip_address = "127.0.0.1"
	return user


## Verify if two SentryUser instances are equal.
func _assert_users_are_equal(user1: SentryUser, user2: SentryUser) -> void:
	assert_str(user1.id).is_equal(user2.id)
	assert_str(user1.email).is_equal(user2.email)
	assert_str(user1.username).is_equal(user2.username)
	assert_str(user1.ip_address).is_equal(user2.ip_address)


## SentryUser data should be correctly saved.
func test_sentry_user_assignment() -> void:
	SentrySDK.set_user(_make_test_user())
	_assert_users_are_equal(SentrySDK.get_user(), _make_test_user())
	assert_bool(SentrySDK.get_user().is_user_valid()).is_true()


## SentryUser data should be properly removed.
func test_sentry_user_remove() -> void:
	SentrySDK.set_user(_make_test_user())
	SentrySDK.remove_user()

	var user := SentrySDK.get_user()
	assert_str(user.email).is_empty()
	assert_str(user.username).is_empty()
	assert_str(user.ip_address).is_empty()
	assert_str(user.id).is_empty()
	assert_bool(user.is_user_valid()).is_false()


## Setting new user data should not contain leftovers from previous data.
func test_sentry_user_no_leftovers() -> void:
	SentrySDK.set_user(_make_test_user())

	# Assign a new user with unique ID.
	var user := SentryUser.new()
	user.generate_new_id()
	SentrySDK.set_user(user)

	user = SentrySDK.get_user()
	assert_str(user.email).is_empty()
	assert_str(user.username).is_empty()
	assert_str(user.ip_address).is_empty()
	assert_str(user.id).is_not_empty()


## SentryUser IP address should contain correct value when IP address is inferred.
func test_sentry_user_ip_inferring() -> void:
	# Assign a new user with IP address inferred.
	var user := SentryUser.new()
	user.infer_ip_address()
	SentrySDK.set_user(user)

	user = SentrySDK.get_user()
	assert_str(user.ip_address).is_equal("{{auto}}")
	assert_str(user.email).is_empty()
	assert_str(user.username).is_empty()


## SentryUser ID generation should produce a unique ID.
func test_sentry_user_id_generation() -> void:
	# Assign a new user with unique ID.
	var user := SentryUser.new()
	user.generate_new_id()
	SentrySDK.set_user(user)

	user = SentrySDK.get_user()
	var id1 := user.id
	assert_int(id1.length()).is_greater(4)

	user.generate_new_id()
	SentrySDK.set_user(user)

	user = SentrySDK.get_user()
	var id2 := user.id
	assert_int(id2.length()).is_greater(4)
	assert_str(id2).is_not_equal(id1) # Newly-generated ID should be different.
