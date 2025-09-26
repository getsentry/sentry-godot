extends SentryTestSuite

func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.send_default_pii = true
		)


func test_device_name() -> void:
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
