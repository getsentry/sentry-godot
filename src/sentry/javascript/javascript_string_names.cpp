#include "sentry/javascript/javascript_string_names.h"

namespace sentry::javascript {

JavaScriptStringNames *JavaScriptStringNames::singleton = nullptr;

void JavaScriptStringNames::create_singleton() {
	singleton = memnew(JavaScriptStringNames);
}

void JavaScriptStringNames::destroy_singleton() {
	memdelete(singleton);
	singleton = nullptr;
}

JavaScriptStringNames::JavaScriptStringNames() {
	Array = StringName("Array");
	Object = StringName("Object");
	Reflect = StringName("Reflect");
	SentryBridge = StringName("SentryBridge");
	addBreadcrumb = StringName("addBreadcrumb");
	attachmentType = StringName("attachmentType");
	captureEvent = StringName("captureEvent");
	captureFeedback = StringName("captureFeedback");
	captureMessage = StringName("captureMessage");
	category = StringName("category");
	close = StringName("close");
	contentType = StringName("contentType");
	contexts = StringName("contexts");
	crashed = StringName("crashed");
	current = StringName("current");
	data = StringName("data");
	deleteProperty = StringName("deleteProperty");
	dist = StringName("dist");
	environment = StringName("environment");
	event_id = StringName("event_id");
	exception = StringName("exception");
	filename = StringName("filename");
	formatted = StringName("formatted");
	getDoubleAsString = StringName("getDoubleAsString");
	id = StringName("id");
	init = StringName("init");
	isEnabled = StringName("isEnabled");
	lastEventId = StringName("lastEventId");
	length = StringName("length");
	level = StringName("level");
	logDebug = StringName("logDebug");
	logError = StringName("logError");
	logFatal = StringName("logFatal");
	logInfo = StringName("logInfo");
	logTrace = StringName("logTrace");
	logWarn = StringName("logWarn");
	logger = StringName("logger");
	main = StringName("main");
	mergeJsonIntoObject = StringName("mergeJsonIntoObject");
	message = StringName("message");
	objectToJson = StringName("objectToJson");
	platform = StringName("platform");
	push = StringName("push");
	pushJsonObjectToArray = StringName("pushJsonObjectToArray");
	release = StringName("release");
	removeContext = StringName("removeContext");
	removeTag = StringName("removeTag");
	removeUser = StringName("removeUser");
	setContext = StringName("setContext");
	setDoubleFromString = StringName("setDoubleFromString");
	setTag = StringName("setTag");
	setUser = StringName("setUser");
	shouldDiscard = StringName("shouldDiscard");
	stacktrace = StringName("stacktrace");
	tags = StringName("tags");
	thread_id = StringName("thread_id");
	threads = StringName("threads");
	timestamp = StringName("timestamp");
	type = StringName("type");
	value = StringName("value");
	values = StringName("values");
}

} //namespace sentry::javascript
