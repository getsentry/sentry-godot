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


## Whether to display name input field.
@export var enable_name_input: bool = true:
	set(value):
		enable_name_input = value
		_update_form()

## Whether to display email input field.
@export var enable_email_input: bool = true:
	set(value):
		enable_email_input = value
		_update_form()

## Enabling this option allows feedback UI to scale for different resolutions.
## Note: The default theme is mastered for 1080p viewport resolution.
@export var auto_scale_ui: bool = true

## Minimum number of words required in the feedback message before the feedback can be submitted.
@export var minimum_words: int = 2:
	set(value):
		minimum_words = value
		_update_form()

## Maximum size constraint for the feedback form (measured in 1080p reference resolution).
@export var maximum_reference_size := Vector2(600, 600)

## Vertical offset from the top edge of the container to position the form (measured in 1080p reference resolution).
@export var top_offset: float = 40.0


@onready var _original_theme: Theme = theme


func _ready():
	_update_form()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventScreenTouch:
		# Hide virtual keyboard when user taps outside the feedback UI.
		DisplayServer.virtual_keyboard_hide()


func _notification(what: int) -> void:
	if what == NOTIFICATION_SORT_CHILDREN:
		_resize_children()


func _update_form() -> void:
	if is_node_ready():
		var form := %UserFeedbackForm
		form.show_logo = show_logo
		form.minimum_words = minimum_words
		form.enable_email_input = enable_email_input
		form.enable_name_input = enable_name_input


## Centers children horizontally at the top of the screen with an offset.
func _resize_children() -> void:
	var sz := get_size();

	# Calculate scale factor
	var scale_xy: float = 1.0
	if auto_scale_ui:
		var vp_size: Vector2 = get_viewport().get_visible_rect().size
		scale_xy = vp_size.y / 1080.0
		_rescale_theme(scale_xy)

	for i in get_child_count():
		var c = get_child(i)
		if c is not Control:
			continue

		var new_sz := Vector2(
			minf(sz.x, maximum_reference_size.x * scale_xy),
			minf(sz.y - top_offset * scale_xy, maximum_reference_size.y * scale_xy))
		new_sz = new_sz.floor()

		var ofs: Vector2 = ((size - new_sz) / 2.0).floor()
		ofs.y = floorf(top_offset * scale_xy)

		# Override size constraints when expand & fill flags are set to use all available space.
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


# *** SCALING FOR DIFFERENT RESOLUTIONS ***

func _rescale_theme(scale_factor: float) -> void:
	if Engine.is_editor_hint() or not _original_theme:
		# Don't make changes to theme in the editor.
		return

	var th: Theme = _original_theme.duplicate()

	th.default_font_size = floori(20 * scale_factor)
	th.set_font_size("font_size", "HeaderMedium", floori(26 * scale_factor))

	# Resize stylebox items
	for theme_type in th.get_stylebox_type_list():
		for sb_name in th.get_stylebox_list(theme_type):
			var sb: StyleBox = th.get_stylebox(sb_name, theme_type)
			if sb is StyleBoxFlat:
				th.set_stylebox(sb_name, theme_type, _scale_stylebox(sb, scale_factor))

	# Resize constants
	for theme_type in th.get_constant_type_list():
		for constant_name in th.get_constant_list(theme_type):
			var c: int = th.get_constant(constant_name, theme_type)
			c = floori(c * scale_factor)
			th.set_constant(constant_name, theme_type, c)

	theme = th


func _scale_stylebox(sb: StyleBox, scale_factor: float) -> StyleBox:
	if sb is StyleBoxFlat:
		var new_sb: StyleBoxFlat = sb.duplicate()

		new_sb.content_margin_bottom = floorf(new_sb.content_margin_bottom * scale_factor)
		new_sb.content_margin_right = floorf(new_sb.content_margin_right * scale_factor)
		new_sb.content_margin_left = floorf(new_sb.content_margin_left * scale_factor)
		new_sb.content_margin_top = floorf(new_sb.content_margin_top * scale_factor)

		new_sb.border_width_bottom = floorf(new_sb.border_width_bottom * scale_factor)
		new_sb.border_width_top = floorf(new_sb.border_width_top * scale_factor)
		new_sb.border_width_left = floorf(new_sb.border_width_left * scale_factor)
		new_sb.border_width_right = floorf(new_sb.border_width_right * scale_factor)

		new_sb.corner_radius_bottom_left = floorf(new_sb.corner_radius_bottom_left * scale_factor)
		new_sb.corner_radius_bottom_right = floorf(new_sb.corner_radius_bottom_right * scale_factor)
		new_sb.corner_radius_top_left = floorf(new_sb.corner_radius_top_left * scale_factor)
		new_sb.corner_radius_top_right = floorf(new_sb.corner_radius_top_right * scale_factor)

		new_sb.expand_margin_bottom = floorf(new_sb.expand_margin_bottom * scale_factor)
		new_sb.expand_margin_left = floorf(new_sb.expand_margin_left * scale_factor)
		new_sb.expand_margin_right = floorf(new_sb.expand_margin_right * scale_factor)
		new_sb.expand_margin_top = floorf(new_sb.expand_margin_top * scale_factor)

		return new_sb
	else:
		return sb
