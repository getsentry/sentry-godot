#pragma once

#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

namespace sentry::javascript {

/**
 * Stores StringName constants for JavaScript SDK implementation.
 * Improves performance by avoiding repeated StringName allocations and hash calculations.
 * Add new names only for repeatedly used strings.
 */
class JavaScriptStringNames {
private:
	friend class JavaScriptSDK;

	static JavaScriptStringNames *singleton;

	static void create_singleton();
	static void destroy_singleton();

	JavaScriptStringNames();

public:
	_FORCE_INLINE_ static JavaScriptStringNames *get_singleton() { return singleton; }

	StringName Array;
	StringName Object;
	StringName Reflect;
	StringName SentryBridge;
	StringName addBreadcrumb;
	StringName attachmentType;
	StringName attributes;
	StringName captureEvent;
	StringName captureFeedback;
	StringName captureMessage;
	StringName category;
	StringName close;
	StringName contentType;
	StringName contexts;
	StringName data;
	StringName deleteProperty;
	StringName dist;
	StringName environment;
	StringName event_id;
	StringName exception;
	StringName filename;
	StringName formatted;
	StringName getDoubleAsString;
	StringName id;
	StringName init;
	StringName isEnabled;
	StringName lastEventId;
	StringName length;
	StringName level;
	StringName logDebug;
	StringName logError;
	StringName logFatal;
	StringName logInfo;
	StringName logTrace;
	StringName logWarn;
	StringName logger;
	StringName mergeJsonIntoObject;
	StringName message;
	StringName objectToJson;
	StringName platform;
	StringName push;
	StringName pushJsonObjectToArray;
	StringName release;
	StringName removeContext;
	StringName removeTag;
	StringName removeUser;
	StringName setContext;
	StringName setDoubleFromString;
	StringName setTag;
	StringName setUser;
	StringName shouldDiscard;
	StringName tags;
	StringName threads;
	StringName timestamp;
	StringName type;
	StringName value;
	StringName values;
};

} //namespace sentry::javascript

#define JAVASCRIPT_SN(m_arg) sentry::javascript::JavaScriptStringNames::get_singleton()->m_arg
