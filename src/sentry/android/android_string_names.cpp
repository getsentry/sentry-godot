#include "sentry/android/android_string_names.h"

namespace sentry::android {

AndroidStringNames *AndroidStringNames::singleton = nullptr;

void AndroidStringNames::create_singleton() {
	singleton = memnew(AndroidStringNames);
}

void AndroidStringNames::destroy_singleton() {
	memdelete(singleton);
	singleton = nullptr;
}

AndroidStringNames::AndroidStringNames() {
	// API methods.
	init = StringName("init");
	close = StringName("close");
	isEnabled = StringName("isEnabled");
	setContext = StringName("setContext");
	removeContext = StringName("removeContext");
	setTag = StringName("setTag");
	removeTag = StringName("removeTag");
	setUser = StringName("setUser");
	removeUser = StringName("removeUser");
	addBreadcrumb = StringName("addBreadcrumb");
	captureMessage = StringName("captureMessage");
	getLastEventId = StringName("getLastEventId");
	captureError = StringName("captureError");
	createEvent = StringName("createEvent");
	releaseEvent = StringName("releaseEvent");
	captureEvent = StringName("captureEvent");
	addFileAttachment = StringName("addFileAttachment");
	addBytesAttachment = StringName("addBytesAttachment");

	// Event methods.
	eventGetId = StringName("eventGetId");
	eventSetMessage = StringName("eventSetMessage");
	eventGetMessage = StringName("eventGetMessage");
	eventSetTimestamp = StringName("eventSetTimestamp");
	eventGetTimestamp = StringName("eventGetTimestamp");
	eventGetPlatform = StringName("eventGetPlatform");
	eventSetLevel = StringName("eventSetLevel");
	eventGetLevel = StringName("eventGetLevel");
	eventSetLogger = StringName("eventSetLogger");
	eventGetLogger = StringName("eventGetLogger");
	eventSetRelease = StringName("eventSetRelease");
	eventGetRelease = StringName("eventGetRelease");
	eventSetDist = StringName("eventSetDist");
	eventGetDist = StringName("eventGetDist");
	eventSetEnvironment = StringName("eventSetEnvironment");
	eventGetEnvironment = StringName("eventGetEnvironment");
	eventSetTag = StringName("eventSetTag");
	eventRemoveTag = StringName("eventRemoveTag");
	eventGetTag = StringName("eventGetTag");
	eventMergeContext = StringName("eventMergeContext");
	eventIsCrash = StringName("eventIsCrash");
	eventToJson = StringName("eventToJson");

	// Exceptions.
	createException = StringName("createException");
	releaseException = StringName("releaseException");
	exceptionAppendStackFrame = StringName("exceptionAppendStackFrame");
	eventAddException = StringName("eventAddException");
	eventGetExceptionCount = StringName("eventGetExceptionCount");
	eventSetExceptionValue = StringName("eventSetExceptionValue");
	eventGetExceptionValue = StringName("eventGetExceptionValue");

	// Breadcrumbs.
	createBreadcrumb = StringName("createBreadcrumb");
	releaseBreadcrumb = StringName("releaseBreadcrumb");
	breadcrumbSetMessage = StringName("breadcrumbSetMessage");
	breadcrumbGetMessage = StringName("breadcrumbGetMessage");
	breadcrumbSetType = StringName("breadcrumbSetType");
	breadcrumbGetType = StringName("breadcrumbGetType");
	breadcrumbSetCategory = StringName("breadcrumbSetCategory");
	breadcrumbGetCategory = StringName("breadcrumbGetCategory");
	breadcrumbSetLevel = StringName("breadcrumbSetLevel");
	breadcrumbGetLevel = StringName("breadcrumbGetLevel");
	breadcrumbSetData = StringName("breadcrumbSetData");
	breadcrumbGetTimestamp = StringName("breadcrumbGetTimestamp");
}

} //namespace sentry::android
