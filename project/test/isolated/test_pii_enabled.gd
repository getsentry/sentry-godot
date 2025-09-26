extends SentryTestSuite

func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.send_default_pii = true
		)


## device.name should be added if PII is enabled
func test_pii_enabled_and_device_name() -> void:
	if OS.get_name() == "macOS":
		# Note: device.name is not reported by Cocoa SDK
		return

	SentrySDK.capture_event(SentrySDK.create_event())

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("Must contain device.name") \
		.at("/contexts/device") \
		.must_contain("name") \
		.verify()

	assert_json(json).describe("device.name should not be empty") \
		.at("/contexts/device/name") \
		.is_not_empty() \
		.verify()


## User interface should contain ip_address set to auto if PII enabled.
func test_pii_enabled_and_user() -> void:
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
