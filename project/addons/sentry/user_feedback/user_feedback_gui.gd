extends Control
## User feedback GUI.
##
## Complete example of User Feedback integration.
##
## Tip: Add "user_feedback_gui.tscn" to your UI CanvasLayer and call show() when needed.
##
## See "user_feedback_form.gd" for more details.


@export var show_logo: bool = true:
	set(value):
		show_logo = value
		%UserFeedbackForm.show_logo = show_logo


func _ready() -> void:
	%UserFeedbackForm.show_logo = show_logo


func _on_user_feedback_form_feedback_submitted(feedback: SentryFeedback) -> void:
	hide()


func _on_user_feedback_form_feedback_cancelled() -> void:
	hide()
