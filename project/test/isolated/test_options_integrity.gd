class_name TestOptionsIntegrity
extends GdUnitTestSuite


static func configure_options(options: SentryOptions) -> void:
	options.release = "1.2.3"
	options.environment = "testing"


func before_test() -> void:
	SentrySDK._set_before_send(_before_send)


func test_options_integrity() -> void:
	var ev := SentrySDK.create_event()
	SentrySDK.capture_event(ev)


func _before_send(ev: SentryEvent) -> SentryEvent:
	assert_str(ev.release).is_equal("1.2.3")
	assert_str(ev.environment).is_equal("testing")
	return null
