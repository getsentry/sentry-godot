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


func test_pii_enabled_and_user() -> void:
	SentrySDK.capture_event(SentrySDK.create_event())

	var json: String = await wait_for_captured_event_json()

	assert_json(json).describe("IP address is set when PII enabled") \
		.at("/user") \
		.must_contain("ip_address") \
		.verify()

	assert_json(json).describe("IP address should be set to {{auto}} when PII enabled") \
		.at("/user/ip_address") \
		.must_be("{{auto}}") \
		.verify()
