extends GutTest
## Test SentryUser class.


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
	assert_eq(user1.id, user2.id)
	assert_eq(user1.email, user2.email)
	assert_eq(user1.username, user2.username)
	assert_eq(user1.ip_address, user2.ip_address)


## Default SentryUser should have non-empty ID initialized to installation_id.
func test_default_user() -> void:
	var user := SentrySDK.get_user()
	assert_ne(user.id, "")
	assert_false(user.is_empty())


## SentryUser data should be correctly saved.
func test_sentry_user_assignment() -> void:
	SentrySDK.set_user(_make_test_user())
	assert_false(SentrySDK.get_user().is_empty())
	_assert_users_are_equal(SentrySDK.get_user(), _make_test_user())


## SentryUser data should be properly removed.
func test_sentry_user_remove() -> void:
	SentrySDK.set_user(_make_test_user())
	SentrySDK.remove_user()

	var user := SentrySDK.get_user()
	assert_eq(user.email, "")
	assert_eq(user.username, "")
	assert_eq(user.ip_address, "")
	assert_eq(user.id, "")
	assert_true(user.is_empty())


## Setting new user data should not contain leftovers from previous data.
func test_sentry_user_no_leftovers() -> void:
	SentrySDK.set_user(_make_test_user())

	# Assign a new user with unique ID.
	var user := SentryUser.new()
	user.generate_new_id()
	SentrySDK.set_user(user)

	user = SentrySDK.get_user()
	assert_eq(user.email, "")
	assert_eq(user.username, "")
	assert_eq(user.ip_address, "")
	assert_ne(user.id, "")


## SentryUser IP address should contain correct value when IP address is inferred.
func test_sentry_user_ip_inferring() -> void:
	# Assign a new user with IP address inferred.
	var user := SentryUser.new()
	user.infer_ip_address()
	SentrySDK.set_user(user)

	user = SentrySDK.get_user()
	assert_eq(user.ip_address, "{{auto}}")
	assert_eq(user.email, "")
	assert_eq(user.username, "")


## SentryUser ID generation should produce a unique ID.
func test_sentry_user_id_generation() -> void:
	# Assign a new user with unique ID.
	var user := SentryUser.new()
	user.generate_new_id()
	SentrySDK.set_user(user)

	user = SentrySDK.get_user()
	var id1 := user.id
	assert_gt(id1.length(), 4)

	user.generate_new_id()
	SentrySDK.set_user(user)

	user = SentrySDK.get_user()
	var id2 := user.id
	assert_gt(id2.length(), 4)
	assert_ne(id2, id1) # Newly-generated ID should be different.
