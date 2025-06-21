#include "sentry_feedback.h"

#include "sentry/simple_bind.h"

void SentryFeedback::_bind_methods() {
	BIND_PROPERTY(SentryFeedback, PropertyInfo(Variant::STRING, "name"), set_name, get_name);
	BIND_PROPERTY(SentryFeedback, PropertyInfo(Variant::STRING, "contact_email"), set_contact_email, get_contact_email);
	BIND_PROPERTY(SentryFeedback, PropertyInfo(Variant::STRING, "message"), set_message, get_message);
	BIND_PROPERTY(SentryFeedback, PropertyInfo(Variant::STRING, "associated_event_id"), set_associated_event_id, get_associated_event_id);
}
