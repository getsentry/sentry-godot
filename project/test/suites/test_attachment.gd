extends GutTest
## Basic tests for the SentryAttachment class.


func test_create_with_path() -> void:
	var attachment := SentryAttachment.create_with_path("user://logs/godot.log")
	attachment.filename = "logfile.txt"
	attachment.content_type = "text/plain"

	assert_true(attachment.bytes.is_empty())
	assert_eq(attachment.path, "user://logs/godot.log")
	assert_eq(attachment.filename, "logfile.txt")
	assert_eq(attachment.content_type, "text/plain")


func test_create_with_bytes() -> void:
	var contents := """
	Hello, world!
	"""
	var bytes: PackedByteArray = contents.to_utf8_buffer()

	var attachment := SentryAttachment.create_with_bytes(bytes, "hello.txt")
	attachment.content_type = "text/plain"

	assert_false(attachment.bytes.is_empty())
	assert_eq(attachment.path, "")
	assert_eq(attachment.filename, "hello.txt")
	assert_eq(attachment.content_type, "text/plain")

	assert_eq(attachment.bytes.size(), bytes.size())
	for i in attachment.bytes.size():
		assert_eq(attachment.bytes[i], bytes[i])
