extends VBoxContainer

@onready var breadcrumb_message: LineEdit = %BreadcrumbMessage
@onready var breadcrumb_category: LineEdit = %BreadcrumbCategory
@onready var tag_key: LineEdit = %TagKey
@onready var tag_value: LineEdit = %TagValue
@onready var context_name: LineEdit = %ContextName
@onready var context_expression: CodeEdit = %ContextExpression
@onready var user_id: LineEdit = %UserID
@onready var username: LineEdit = %Username
@onready var email: LineEdit = %Email
@onready var infer_ip: CheckBox = %InferIP


func _ready() -> void:
	_init_user_info()


func _init_user_info() -> void:
	var user := SentryUser.create_default()
	SentrySDK.set_user(user)

	username.text = user.username
	email.text = user.email
	user_id.text = user.id


func _on_set_user_button_pressed() -> void:
	DemoOutput.print_info("Setting user info...")
	var user := SentryUser.new()
	user.id = user_id.text
	user.username = username.text
	user.email = email.text
	if infer_ip.button_pressed:
		user.infer_ip_address()
	SentrySDK.set_user(user)
	DemoOutput.print_extra(str(user))


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
