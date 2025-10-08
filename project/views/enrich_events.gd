extends VBoxContainer

@onready var breadcrumb_message: LineEdit = %BreadcrumbMessage
@onready var breadcrumb_category: LineEdit = %BreadcrumbCategory
@onready var tag_key: LineEdit = %TagKey
@onready var tag_value: LineEdit = %TagValue
@onready var context_name: LineEdit = %ContextName
@onready var context_expression: CodeEdit = %ContextExpression


func _ready() -> void:
	SentrySDK.logger.debug("Starting %s at %d usec", ["SentrySDK", Time.get_ticks_usec()], {
		"hello": "world!",
		"meaning.of.life": 42
	})

	SentrySDK.logger.info("Hello")


func _on_add_breadcrumb_button_pressed() -> void:
	var crumb := SentryBreadcrumb.create(breadcrumb_message.text)
	crumb.category = breadcrumb_category.text
	SentrySDK.add_breadcrumb(crumb)
	DemoOutput.print_info("Breadcrumb added.")


func _on_add_tag_button_pressed() -> void:
	SentrySDK.set_tag(tag_key.text, tag_value.text)
	if not tag_key.text.is_empty():
		DemoOutput.print_info("Tag added.")


func _on_set_context_pressed() -> void:
	if context_name.text.is_empty():
		DemoOutput.print_info("Please provide a name for the context.")
		return

	# Filter out comments because Expression doesn't support them.
	var expr_lines := Array(context_expression.text.split("\n")).filter(
			func(s: String): return not s.begins_with("#"))
	var filtered_expression := "".join(expr_lines)

	# Parsing expression dictionary.
	var expr := Expression.new()
	var error: Error = expr.parse(filtered_expression)
	if error == OK:
		var result = expr.execute()
		if typeof(result) == TYPE_DICTIONARY:
			# Adding context.
			SentrySDK.set_context(context_name.text, result)
			DemoOutput.print_info("Context added.")
		else:
			DemoOutput.print_err("Failed set context: Dictionary is expected, but found: " + type_string(typeof(result)))
	else:
		DemoOutput.print_err("Failed to parse expression: " + expr.get_error_text())


func _on_attach_button_pressed() -> void:
	var content: String = %AttachmentContent.text
	var bytes: PackedByteArray = content.to_utf8_buffer()
	var attachment := SentryAttachment.create_with_bytes(bytes, "hello.txt")
	attachment.content_type = "text/plain"
	SentrySDK.add_attachment(attachment)
	DemoOutput.print_info("Attachment added.")
