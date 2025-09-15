extends SentryTestSuite
## Basic tests for the SentryAttachment class.


func test_create_with_path() -> void:
	var attachment := SentryAttachment.create_with_path("user://logs/godot.log")
	attachment.filename = "logfile.txt"
	attachment.content_type = "text/plain"

	assert_array(attachment.bytes).is_empty()
	assert_str(attachment.path).is_equal("user://logs/godot.log")
	assert_str(attachment.filename).is_equal("logfile.txt")
	assert_str(attachment.content_type).is_equal("text/plain")


func test_create_with_bytes() -> void:
	var contents := """
	Hello, world!
	"""
	var bytes: PackedByteArray = contents.to_utf8_buffer()

	var attachment := SentryAttachment.create_with_bytes(bytes, "hello.txt")
	attachment.content_type = "text/plain"

	assert_array(attachment.bytes).is_not_empty()
	assert_str(attachment.path).is_empty()
	assert_str(attachment.filename).is_equal("hello.txt")
	assert_str(attachment.content_type).is_equal("text/plain")

	assert_int(attachment.bytes.size()).is_equal(bytes.size())
	for i in attachment.bytes.size():
		assert_int(attachment.bytes[i]).is_equal(bytes[i])
