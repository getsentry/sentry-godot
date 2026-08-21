#pragma once

#include "sentry/sentry_feedback_impl.h"

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

// Godot-exported representation of user feedback.
// Implementations are provided by SentryFeedbackImpl subclasses.
class SentryFeedback : public RefCounted {
	GDCLASS(SentryFeedback, RefCounted);

private:
	SentryFeedbackImpl *_impl;

protected:
	static void _bind_methods();

public:
	String get_name() const { return _impl->get_name(); }
	void set_name(const String &p_name) { _impl->set_name(p_name); }

	String get_contact_email() const { return _impl->get_contact_email(); }
	void set_contact_email(const String &p_contact_email) { _impl->set_contact_email(p_contact_email); }

	String get_message() const { return _impl->get_message(); }
	void set_message(const String &p_message) { _impl->set_message(p_message); }

	String get_associated_event_id() const { return _impl->get_associated_event_id(); }
	void set_associated_event_id(const String &p_associated_event_id) { _impl->set_associated_event_id(p_associated_event_id); }

	SentryFeedback();
	SentryFeedback(SentryFeedbackImpl *p_impl);
	~SentryFeedback();
};

} //namespace sentry
