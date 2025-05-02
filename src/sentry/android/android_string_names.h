#ifndef SENTRY_ANDROID_STRING_NAMES_H
#define SENTRY_ANDROID_STRING_NAMES_H

#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

namespace sentry {

/**
 * Stores StringName constants for Android SDK implementation.
 * Improves performance by avoiding repeated StringName allocations and hash calculations.
 * Add new names only for repeatedly used strings.
 */
class AndroidStringNames {
private:
	friend class AndroidSDK;

	static AndroidStringNames *singleton;

	static void create_singleton();
	static void destroy_singleton();

	AndroidStringNames();

public:
	_FORCE_INLINE_ static AndroidStringNames *get_singleton() { return singleton; }

	// API methods.
	StringName addGlobalAttachment;
	StringName setContext;
	StringName removeContext;
	StringName setTag;
	StringName removeTag;
	StringName setUser;
	StringName removeUser;
	StringName addBreadcrumb;
	StringName captureMessage;
	StringName getLastEventId;
	StringName captureError;
	StringName createEvent;
	StringName releaseEvent;
	StringName captureEvent;

	// Event methods.
	StringName eventGetId;
	StringName eventSetMessage;
	StringName eventGetMessage;
	StringName eventSetTimestamp;
	StringName eventGetTimestamp;
	StringName eventGetPlatform;
	StringName eventSetLevel;
	StringName eventGetLevel;
	StringName eventSetLogger;
	StringName eventGetLogger;
	StringName eventSetRelease;
	StringName eventGetRelease;
	StringName eventSetDist;
	StringName eventGetDist;
	StringName eventSetEnvironment;
	StringName eventGetEnvironment;
	StringName eventSetTag;
	StringName eventRemoveTag;
	StringName eventGetTag;

	// Exceptions.
	StringName createException;
	StringName releaseException;
	StringName exceptionAppendStackFrame;
	StringName eventAddException;
};

} // namespace sentry

#define ANDROID_SN(m_arg) sentry::AndroidStringNames::get_singleton()->m_arg

#endif // SENTRY_ANDROID_STRING_NAMES_H
