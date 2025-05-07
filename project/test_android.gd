extends Node2D


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	test_this()


func test_this():
	if Engine.has_singleton("SentryAndroidGodotPlugin"):
		print("ANDROID: Singleton found")
		#var singleton = Engine.get_singleton("SentryAndroidGodotPlugin")
		#var hello = singleton.testThis()
		var cl = JavaClassWrapper.wrap("io.sentry.godotplugin.Hello")
		print(cl)


func test_android() -> void:
	if Engine.has_singleton("SentryAndroidGodotPlugin"):
		print("ANDROID: Singleton found")
		var singleton = Engine.get_singleton("SentryAndroidGodotPlugin")
		singleton.initialize()
		print("ANDROID: Sentry Android initialized")

		test_message_capture(singleton)
		#test_event(singleton)

	else:
		print("ANDROID: No singleton")


func test_message_capture(singleton) -> void:
	singleton.setContext("ship", {"speed": 200, "guns": "lasers"})
	singleton.setTag("biome", "nebula")
	singleton.addBreadcrumb("Hello from GDScript!", "gdscript", 3, "default", {"drink": "tea of course", "email": "user@example.com"})
	var id = singleton.captureMessage("Hey, it's Android knocking!", 3)
	print("ANDROID: Message captured: ", id)


func test_event(singleton) -> void:
	var id = singleton.createEvent()
	print("ANDROID: Creating event: ", id)

	singleton.eventSetMessage(id, "Android custom event")
	print("ANDROID:     message: ", singleton.eventGetMessage(id))

	var ts = Time.get_datetime_string_from_system()
	singleton.eventSetTimestamp(id, ts)
	print("ANDROID:     timestamp: ", singleton.eventGetTimestamp(id), " Expected: ", ts)

	print("ANDROID:     platform: ", singleton.eventGetPlatform(id))

	print("ANDROID:     default level: ", singleton.eventGetLevel(id))
	singleton.eventSetLevel(id, SentrySDK.LEVEL_DEBUG)
	print("ANDROID:     level: ", singleton.eventGetLevel(id))

	singleton.eventSetLogger(id, "test-logger")
	print("ANDROID:     logger: ", singleton.eventGetLogger(id))

	singleton.eventSetRelease(id, "demo@0.0.1")
	print("ANDROID:     release: ", singleton.eventGetRelease(id))

	singleton.eventSetDist(id, "disto")
	print("ANDROID:     dist: ", singleton.eventGetDist(id))

	singleton.eventSetEnvironment(id, "test")
	print("ANDROID:     environment: ", singleton.eventGetEnvironment(id))

	singleton.eventSetTag(id, "something", "new")
	print("ANDROID:     tag: ", singleton.eventGetTag(id, "something"))

	var captured_id = singleton.captureEvent(id)
	print("ANDROID:     Event captured: ", captured_id)
	singleton.releaseEvent(id)
	print("ANDROID:     Event released: ", id)
