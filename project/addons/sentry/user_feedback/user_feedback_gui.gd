@tool
extends Container
## Complete example of User Feedback integration.
##
## Usage: Add "user_feedback_gui.tscn" to your UI scene and call show() when needed.
##
## See "user_feedback_form.gd" for more details.


## Whether to display Sentry logo in the top right corner.
@export var logo_visible: bool = true:
	set(value):
		logo_visible = value
		_update_form()


## Whether to display name input field.
@export var name_visible: bool = true:
	set(value):
		name_visible = value
		_update_form()

## Whether to display email input field.
@export var email_visible: bool = true:
	set(value):
		email_visible = value
		_update_form()

## Minimum number of words required in the feedback message before the feedback can be submitted.
@export var minimum_words: int = 2:
	set(value):
		minimum_words = value
		_update_form()

## Maximum size constraint for the feedback form (measured in reference resolution).
@export var maximum_form_size := Vector2(600, 600)

## Vertical offset from the top edge of the container to position the form (measured in reference resolution).
@export var top_offset: float = 40.0


@export_group("Auto Scale UI", "auto_scale")

## Enabling this option allows feedback UI to scale for different resolutions.
## Note: The default theme is mastered for 1080p viewport resolution.
@export var auto_scale_enable: bool = true

## Master resolution used as reference for UI scaling calculations.
## When auto_scale_enable is ON, the UI will scale proportionally based on
## the ratio between the current viewport height and this resolution.
@export var auto_scale_master_resolution: int = 1080


@onready var _original_theme: Theme = theme


func _ready():
	_update_form()


func _gui_input(event: InputEvent) -> void:
	if not visible:
		return
	if event is InputEventScreenTouch:
		# Hide virtual keyboard when user taps outside the feedback UI.
		DisplayServer.virtual_keyboard_hide()


func _notification(what: int) -> void:
	if what == NOTIFICATION_SORT_CHILDREN:
		_resize_children()


func _update_form() -> void:
	if is_node_ready():
		var form := %UserFeedbackForm
		form.logo_visible = logo_visible
		form.email_visible = email_visible
		form.name_visible = name_visible
		form.minimum_words = minimum_words


## Centers children horizontally at the top of the screen with an offset,
## and optionally rescales for actual viewport resolution.
func _resize_children() -> void:
	var sz := get_size();

	# Calculate scale factor
	var scale_xy: float = 1.0
	if auto_scale_enable:
		var vp_size: Vector2 = get_viewport().get_visible_rect().size
		scale_xy = vp_size.y / auto_scale_master_resolution
		_rescale_theme(scale_xy)

	for i in get_child_count():
		var c = get_child(i)
		if c is not Control:
			continue

		var new_sz := Vector2(
			minf(sz.x, maximum_form_size.x * scale_xy),
			minf(sz.y - top_offset * scale_xy, maximum_form_size.y * scale_xy))
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

	var font_size: int = th.default_font_size
	th.default_font_size = ceili(font_size * scale_factor)
	th.set_font_size("font_size", "HeaderMedium", ceili(font_size * 1.3 * scale_factor))

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
			c = ceili(c * scale_factor)
			th.set_constant(constant_name, theme_type, c)

	theme = th


func _scale_stylebox(sb: StyleBox, scale_factor: float) -> StyleBox:
	if sb is StyleBoxFlat:
		var new_sb: StyleBoxFlat = sb.duplicate()

		new_sb.content_margin_bottom = ceilf(new_sb.content_margin_bottom * scale_factor)
		new_sb.content_margin_right = ceilf(new_sb.content_margin_right * scale_factor)
		new_sb.content_margin_left = ceilf(new_sb.content_margin_left * scale_factor)
		new_sb.content_margin_top = ceilf(new_sb.content_margin_top * scale_factor)

		new_sb.border_width_bottom = ceilf(new_sb.border_width_bottom * scale_factor)
		new_sb.border_width_top = ceilf(new_sb.border_width_top * scale_factor)
		new_sb.border_width_left = ceilf(new_sb.border_width_left * scale_factor)
		new_sb.border_width_right = ceilf(new_sb.border_width_right * scale_factor)

		new_sb.corner_radius_bottom_left = ceilf(new_sb.corner_radius_bottom_left * scale_factor)
		new_sb.corner_radius_bottom_right = ceilf(new_sb.corner_radius_bottom_right * scale_factor)
		new_sb.corner_radius_top_left = ceilf(new_sb.corner_radius_top_left * scale_factor)
		new_sb.corner_radius_top_right = ceilf(new_sb.corner_radius_top_right * scale_factor)

		new_sb.expand_margin_bottom = ceilf(new_sb.expand_margin_bottom * scale_factor)
		new_sb.expand_margin_left = ceilf(new_sb.expand_margin_left * scale_factor)
		new_sb.expand_margin_right = ceilf(new_sb.expand_margin_right * scale_factor)
		new_sb.expand_margin_top = ceilf(new_sb.expand_margin_top * scale_factor)

		return new_sb
	else:
		return sb
