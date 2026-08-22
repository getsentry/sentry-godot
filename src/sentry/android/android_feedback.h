#pragma once

#include "sentry/sentry_feedback_impl.h"

#include <godot_cpp/classes/object.hpp>

using namespace godot;

namespace sentry::android {

class AndroidFeedback : public SentryFeedbackImpl {
	SENTRY_CASTABLE(AndroidFeedback, SentryFeedbackImpl);

private:
	Object *android_plugin = nullptr;
	int32_t handle = 0;

public:
	virtual void set_name(const String &p_name) override;
	virtual String get_name() const override;

	virtual void set_contact_email(const String &p_contact_email) override;
	virtual String get_contact_email() const override;

	virtual void set_message(const String &p_message) override;
	virtual String get_message() const override;

	virtual void set_associated_event_id(const String &p_associated_event_id) override;
	virtual String get_associated_event_id() const override;

	AndroidFeedback(Object *p_android_plugin, int32_t p_feedback_handle);
	virtual ~AndroidFeedback() override;
};

} //namespace sentry::android
