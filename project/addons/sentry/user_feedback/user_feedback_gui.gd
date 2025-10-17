@tool
extends Container
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

## Limit the maximum size that the form can have.
@export var maximum_size := Vector2(600, 600)

@export var top_offset: float = 40.0

func _ready():
	_update_form()


func _notification(what: int) -> void:
	if what == NOTIFICATION_SORT_CHILDREN:
		_resize_children()


func _update_form():
	if is_node_ready():
		var form := %UserFeedbackForm
		form.show_logo = show_logo
		form.minimum_words = minimum_words


## Centers children horizontally at the top of the screen with an offset.
func _resize_children() -> void:
	var sz := get_size();

	for i in get_child_count():
		var c = get_child(i)
		if c is not Control:
			continue

		var new_sz := Vector2(minf(sz.x, maximum_size.x), minf(sz.y, maximum_size.y))
		var ofs: Vector2 = ((size - new_sz) / 2.0).floor()
		ofs.y = top_offset

		if c.size_flags_vertical == SIZE_EXPAND_FILL:
			new_sz.y = size.y
			ofs.y = 0

		if c.size_flags_horizontal == SIZE_EXPAND_FILL:
			new_sz.x = size.x
			ofs.x = 0

		fit_child_in_rect(c, Rect2(ofs, new_sz));


func _on_user_feedback_form_feedback_submitted(feedback: SentryFeedback) -> void:
	hide()


func _on_user_feedback_form_feedback_cancelled() -> void:
	hide()
