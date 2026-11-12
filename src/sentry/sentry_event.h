#pragma once

#include "sentry/level.h"
#include "sentry/sentry_timestamp.h"
#include "sentry/sentry_user.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/pair.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry {

class SentryEventImpl;

// Godot-facing event backed by a platform-specific implementation.
class SentryEvent : public RefCounted {
	GDCLASS(SentryEvent, RefCounted);

public:
	// Represents a frame of a stack trace.
	struct StackFrame {
		String filename;
		String function;
		int lineno = -1;
		bool in_app = true;
		String platform;
		String context_line;
		Vector<Pair<String, Variant>> vars;
		PackedStringArray pre_context;
		PackedStringArray post_context;
	};

	struct Exception {
		String type;
		String value;
		Vector<StackFrame> frames;
	};

private:
	SentryEventImpl *_impl = nullptr;

protected:
	static void _bind_methods();

public:
	String get_id() const;

	void set_message(const String &p_message);
	String get_message() const;

	void set_timestamp(const Ref<SentryTimestamp> &p_timestamp);
	Ref<SentryTimestamp> get_timestamp() const;

	String get_platform() const;

	void set_level(sentry::Level p_level);
	sentry::Level get_level() const;

	void set_logger(const String &p_logger);
	String get_logger() const;

	void set_release(const String &p_release);
	String get_release() const;

	void set_dist(const String &p_dist);
	String get_dist() const;

	void set_environment(const String &p_environment);
	String get_environment() const;

	void set_tag(const String &p_key, const String &p_value);
	void remove_tag(const String &p_key);
	String get_tag(const String &p_key);

	void set_user(const Ref<SentryUser> &p_user);

	void set_fingerprint(const PackedStringArray &p_fingerprint);

	void set_context(const String &p_key, const Dictionary &p_value);
	void merge_context(const String &p_key, const Dictionary &p_value);

	void add_exception(const Exception &p_exception);

	int get_exception_count() const;
	void set_exception_value(int p_index, const String &p_value);
	String get_exception_value(int p_index) const;

	bool is_crash() const;

	String to_json() const;

	static Ref<SentryEvent> create_noop();

	_FORCE_INLINE_ SentryEventImpl *get_implementation() const { return _impl; }

	SentryEvent();
	SentryEvent(SentryEventImpl *p_impl);
	~SentryEvent();
};

} // namespace sentry
