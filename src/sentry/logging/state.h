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

// Controls whether Godot logger messages should be forwarded to Sentry Logs.
// When true, Godot logger messages are suppressed and not sent to Logs.
// This flag is used by SentryLogger to send log messages with custom structure
// while avoiding duplicate messages that would otherwise be produced by the
// Godot logging system.
extern thread_local bool skip_logging_messages;

} //namespace sentry::logging
