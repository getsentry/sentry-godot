extends GdUnitTestSuite
## Basic tests for the SentryAttachment class.


func test_create_with_path() -> void:
	var attachment := SentryAttachment.create_with_path(
		"res://example_configuration.gd",
		"config.gd",
		"text/x-gdscript"
	)
	assert_array(attachment.bytes).is_empty()
	assert_str(attachment.path).is_equal("res://example_configuration.gd")
	assert_str(attachment.filename).is_equal("config.gd")
	assert_str(attachment.content_type).is_equal("text/x-gdscript")


func test_create_with_bytes() -> void:
	var contents := """
	Hello, world!
	"""
	var bytes: PackedByteArray = contents.to_utf8_buffer()

	var attachment := SentryAttachment.create_with_bytes(
		bytes, "hello.txt", "text/plain")

	assert_array(attachment.bytes).is_not_empty()
	assert_str(attachment.path).is_empty()
	assert_str(attachment.filename).is_equal("hello.txt")
	assert_str(attachment.content_type).is_equal("text/plain")

	assert_int(attachment.bytes.size()).is_equal(bytes.size())
	for i in attachment.bytes.size():
		assert_int(attachment.bytes[i]).is_equal(bytes[i])
