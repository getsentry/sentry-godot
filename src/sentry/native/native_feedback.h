#pragma once

#include "sentry/sentry_feedback_impl.h"

#include <sentry.h>

namespace sentry::native {

class NativeFeedback : public SentryFeedbackImpl {
	SENTRY_CASTABLE(NativeFeedback, SentryFeedbackImpl);

private:
	sentry_value_t native_feedback;

public:
	virtual void set_name(const String &p_name) override;
	virtual String get_name() const override;

	virtual void set_contact_email(const String &p_contact_email) override;
	virtual String get_contact_email() const override;

	virtual void set_message(const String &p_message) override;
	virtual String get_message() const override;

	virtual void set_associated_event_id(const String &p_associated_event_id) override;
	virtual String get_associated_event_id() const override;

	NativeFeedback(sentry_value_t p_feedback);
	virtual ~NativeFeedback() override;
};

} //namespace sentry::native
