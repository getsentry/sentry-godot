#ifndef INTERNAL_SDK_H
#define INTERNAL_SDK_H

#include "sentry/level.h"
#include "sentry/log_level.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_event.h"
#include "sentry/sentry_feedback.h"
#include "sentry/sentry_user.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry {

// Interface for SDKs used internally.
class InternalSDK {
public:
	enum Capabilities : int64_t {
		SUPPORTS_EARLY_INIT = (1LL << 0),
		// SUPPORTS_RUNNING_DOOM = (1LL << 1),
		// SUPPORTS_LANDING_ON_THE_MOON = (1LL << 2)
		SUPPORTS_ALL = ~0LL
	};

public:
	virtual BitField<Capabilities> get_capabilities() const { return Capabilities::SUPPORTS_ALL; }

	virtual void set_context(const String &p_key, const Dictionary &p_value) = 0;
	virtual void remove_context(const String &p_key) = 0;

	virtual void set_tag(const String &p_key, const String &p_value) = 0;
	virtual void remove_tag(const String &p_key) = 0;

	virtual void set_user(const Ref<SentryUser> &p_user) = 0;
	virtual void remove_user() = 0;

	virtual Ref<SentryBreadcrumb> create_breadcrumb() = 0;
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) = 0;

	virtual void log(LogLevel p_level, const String &p_body, const Dictionary &p_attributes = Dictionary()) = 0;

	virtual String capture_message(const String &p_message, Level p_level) = 0;
	virtual String get_last_event_id() = 0;

	virtual Ref<SentryEvent> create_event() = 0;
	virtual String capture_event(const Ref<SentryEvent> &p_event) = 0;

	virtual void capture_feedback(const Ref<SentryFeedback> &p_feedback) = 0;

	virtual void add_attachment(const Ref<SentryAttachment> &p_attachment) = 0;

	virtual void init(const PackedStringArray &p_global_attachments, const Callable &p_configuration_callback) = 0;
	virtual void close() = 0;
	virtual bool is_enabled() const = 0;

	virtual ~InternalSDK() = default;
};

} //namespace sentry

#endif // INTERNAL_SDK_H
