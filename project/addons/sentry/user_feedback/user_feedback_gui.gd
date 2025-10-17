extends Control
## Complete example of User Feedback integration.
##
## Usage: Add "user_feedback_gui.tscn" to your UI scene and call show() when needed.
##
## See "user_feedback_form.gd" for more details.


## Whether to display Sentry logo in the top right corner.
@export var show_logo: bool = true:
	set(value):
		show_logo = value
		_update_form()


## Minimum number of words required in the feedback message before the feedback can be submitted.
@export var minimum_words: int = 2:
	set(value):
		minimum_words = value
		_update_form()


func _ready():
	_update_form()


func _update_form():
	if is_node_ready():
		%UserFeedbackForm.show_logo = show_logo
		%UserFeedbackForm.minimum_words = minimum_words


func _on_user_feedback_form_feedback_submitted(feedback: SentryFeedback) -> void:
	hide()


func _on_user_feedback_form_feedback_cancelled() -> void:
	hide()
