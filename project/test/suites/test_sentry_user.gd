extends SentryTestSuite
## Test SentryUser class.


## Make a SentryUser instance with pre-filled test data.
func _make_test_user() -> SentryUser:
	var user := SentryUser.new()
	user.id = "custom_id"
	user.email = "bob@example.com"
	user.username = "bob"
	user.ip_address = "127.0.0.1"
	return user


## Default SentryUser should have non-empty ID initialized to installation_id.
func test_default_user() -> void:
	var user := SentrySDK.get_user()
	assert_str(user.id).is_not_empty()
	assert_bool(user.is_empty()).is_false()


## SentryUser data should be correctly saved.
func test_sentry_user_assignment() -> void:
	var expected: SentryUser = _make_test_user()

	SentrySDK.set_user(_make_test_user())

	var actual: SentryUser = SentrySDK.get_user()

	assert_bool(actual.is_empty()).is_false()
	assert_str(expected.id).is_equal(actual.id)
	assert_str(expected.email).is_equal(actual.email)
	assert_str(expected.username).is_equal(actual.username)
	assert_str(expected.ip_address).is_equal(actual.ip_address)


## SentryUser data should be properly removed.
func test_sentry_user_remove() -> void:
	SentrySDK.set_user(_make_test_user())
	SentrySDK.remove_user()

	var user := SentrySDK.get_user()
	assert_str(user.email).is_empty()
	assert_str(user.username).is_empty()
	assert_str(user.ip_address).is_empty()
	assert_str(user.id).is_empty()
	assert_bool(user.is_empty()).is_true()


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
