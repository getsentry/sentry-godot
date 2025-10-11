#pragma once

namespace sentry::logging {

// Whether the engine logger is currently processing a message.
extern thread_local bool in_message_logging;

// Sets a thread-local flag indicating that we are currently logging a message.
// This prevents debug output from being logged within another log operation,
// which can cause errors in Godot.
struct MessageScope {
	MessageScope() { in_message_logging = true; }
	~MessageScope() { in_message_logging = false; }
};

} //namespace sentry::logging
