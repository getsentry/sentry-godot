extends PanelContainer
## User Feedback Form
##
## This is a reference user feedback form implementation.
##
## Tip: Copy folder with this feedback form into your project and customize it to your needs.


signal feedback_submitted(feedback: SentryFeedback)
signal feedback_cancelled()

@export var show_logo: bool = true:
	set(value):
		show_logo = value
		_update_logo()

@onready var message_edit: TextEdit = %MessageEdit
@onready var name_edit: LineEdit = %NameEdit
@onready var email_edit: LineEdit = %EmailEdit
@onready var submit_button: Button = %SubmitButton


func _ready() -> void:
	_update_logo()


func _update_logo() -> void:
	%Logo.visible = show_logo


func _on_submit_button_pressed() -> void:
	var feedback := SentryFeedback.new()
	feedback.message = message_edit.text
	feedback.name = name_edit.text
	feedback.contact_email = email_edit.text

	SentrySDK.capture_feedback(feedback)

	feedback_submitted.emit(feedback)

	# Reset feedback message
	message_edit.text = ""


func _on_message_edit_text_changed() -> void:
	var message: String = message_edit.text
	submit_button.disabled = _count_words(message) < 2


func _count_words(text: String) -> int:
	var words: PackedStringArray = text.strip_edges().split(" ", false)
	var clean_words: PackedStringArray = []
	for word in words:
		if word.strip_edges() != "":
			clean_words.append(word)
	return clean_words.size()


func _on_cancel_button_pressed() -> void:
	feedback_cancelled.emit()
