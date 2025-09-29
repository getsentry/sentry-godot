extends SentryTestSuite
## Test `send_default_pii` option enabled


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.send_default_pii = true
		)


## User interface should contain ip_address set to auto if PII enabled.
func test_pii_enabled_and_default_user_ip() -> void:
	SentrySDK.capture_event(SentrySDK.create_event())

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("User interface must contain ip_address") \
		.at("/user") \
		.must_contain("ip_address") \
		.verify()

	assert_json(json).describe("ip_address should be set to {{auto}}") \
		.at("/user/ip_address") \
		.must_be("{{auto}}") \
		.verify()
