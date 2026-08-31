#pragma once

#include "sentry/javascript/javascript_interop.h"
#include "sentry/sentry_feedback_impl.h"

namespace sentry::javascript {

// Backed by the JS feedback context object of an event, which is guaranteed to be valid.
class JavaScriptFeedback : public SentryFeedbackImpl {
	SENTRY_CASTABLE(JavaScriptFeedback, SentryFeedbackImpl);

private:
	JSObjectPtr js_obj;

public:
	virtual void set_name(const String &p_name) override;
	virtual String get_name() const override;

	virtual void set_contact_email(const String &p_contact_email) override;
	virtual String get_contact_email() const override;

	virtual void set_message(const String &p_message) override;
	virtual String get_message() const override;

	virtual void set_associated_event_id(const String &p_associated_event_id) override;
	virtual String get_associated_event_id() const override;

	JavaScriptFeedback(const JSObjectPtr &p_js_feedback_object);
};

} //namespace sentry::javascript
