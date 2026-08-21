#pragma once

#include "sentry/castable.h"

#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry {

// Base class for user feedback implementations; see Godot-facing SentryFeedback.
// Splitting the implementation from SentryFeedback keeps SentryFeedback.new()
// working, which an abstract class with a create() factory would break.
// Kept as a pure C++ class instead of a Godot class to avoid ClassDB
// registration and reduce overhead.
// Lifetime governed by SentryFeedback.
class SentryFeedbackImpl : public Castable {
	SENTRY_CASTABLE(SentryFeedbackImpl, Castable);

public:
	virtual void set_name(const String &p_name) = 0;
	virtual String get_name() const = 0;

	virtual void set_contact_email(const String &p_contact_email) = 0;
	virtual String get_contact_email() const = 0;

	virtual void set_message(const String &p_message) = 0;
	virtual String get_message() const = 0;

	virtual void set_associated_event_id(const String &p_associated_event_id) = 0;
	virtual String get_associated_event_id() const = 0;

	virtual ~SentryFeedbackImpl() = default;
};

// Holds the submitted fields in memory until a backend reads them at capture time.
class PlainFeedback : public SentryFeedbackImpl {
	SENTRY_CASTABLE(PlainFeedback, SentryFeedbackImpl);

private:
	String name;
	String contact_email;
	String message;
	String associated_event_id;

public:
	virtual void set_name(const String &p_name) override { name = p_name; }
	virtual String get_name() const override { return name; }

	virtual void set_contact_email(const String &p_contact_email) override { contact_email = p_contact_email; }
	virtual String get_contact_email() const override { return contact_email; }

	virtual void set_message(const String &p_message) override { message = p_message; }
	virtual String get_message() const override { return message; }

	virtual void set_associated_event_id(const String &p_associated_event_id) override { associated_event_id = p_associated_event_id; }
	virtual String get_associated_event_id() const override { return associated_event_id; }
};

} //namespace sentry
