extends SentryTestSuite
## Test `send_default_pii` option disabled


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.send_default_pii = false
		)


## User interface must not contain ip_adress if PII disabled.
func test_pii_disabled_and_default_user_ip() -> void:
	SentrySDK.capture_event(SentrySDK.create_event())

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("User interface must NOT contain ip_address") \
		.at("/user") \
		.must_not_contain("ip_address") \
		.verify()
