extends GdUnitTestSuite
## Basic tests for the SentryAttachment class.


func test_create_with_path() -> void:
	var attachment := SentryAttachment.create_with_path(
		"res://example_configuration.gd",
		"config.gd",
		"text/x-gdscript"
	)
	assert_str(attachment.path).is_equal("res://example_configuration.gd")
	assert_str(attachment.filename).is_equal("config.gd")
	assert_str(attachment.content_type).is_equal("text/x-gdscript")
