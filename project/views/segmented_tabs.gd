class_name SegmentedTabs
extends MarginContainer
## A segmented tab bar that keeps its buttons synchronized with a TabContainer.


@export var tab_container_path: NodePath = ^"../TabContainer"

@onready var _tab_container: TabContainer = get_node(tab_container_path)
@onready var _tab_buttons: Array[Button] = [
	%EnrichTab,
	%CaptureTab,
	%ToolsTab,
]


func _ready() -> void:
	for index in _tab_buttons.size():
		_tab_buttons[index].pressed.connect(_on_tab_pressed.bind(index))
	_tab_container.tab_changed.connect(_select_tab)
	_select_tab(_tab_container.current_tab)


func _on_tab_pressed(index: int) -> void:
	_tab_container.current_tab = index


func _select_tab(index: int) -> void:
	for button_index in _tab_buttons.size():
		var button: Button = _tab_buttons[button_index]
		var selected := button_index == index
		button.set_pressed_no_signal(selected)
