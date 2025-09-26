extends SentryTestSuite

func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.send_default_pii = false
		)


## device.name should NOT be added if PII disabled
func test_pii_disabled_and_device_name() -> void:
	SentrySDK.capture_event(SentrySDK.create_event())

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("Must NOT contain device.name") \
		.at("/contexts/device") \
		.must_not_contain("name") \
		.verify()


## User IP should not be set to {{auto}} if PII disabled
func test_pii_disabled_and_user() -> void:
	SentrySDK.capture_event(SentrySDK.create_event())

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("IP address is set when PII enabled") \
		.at("/user") \
		.must_not_contain("ip_address") \
		.verify()
