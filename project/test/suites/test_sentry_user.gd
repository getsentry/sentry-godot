extends SentryTestSuite
## Test SentryUser class.


## SentryUser IP address should contain correct value when IP address is inferred.
func test_sentry_user_ip_inferring() -> void:
	var user := SentryUser.new()
	user.infer_ip_address()
	assert_str(user.ip_address).is_equal("{{auto}}")


## SentryUser ID generation should produce a unique ID.
func test_sentry_user_id_generation() -> void:
	var user := SentryUser.new()

	user.generate_new_id()
	var id1 := user.id
	user.generate_new_id()
	var id2 := user.id

	assert_int(id1.length()).is_greater(4)
	assert_int(id2.length()).is_greater(4)
	assert_str(id2).is_not_equal(id1).override_failure_message("Newly-generated ID should be different")
