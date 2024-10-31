#ifndef DISABLED_SDK_H
#define DISABLED_SDK_H

#include "internal_sdk.h"

namespace sentry {

// Internal SDK that does nothing.
class DisabledSDK : public InternalSDK {
	virtual void set_context(const String &p_key, const Dictionary &p_value) override {}
	virtual void remove_context(const String &p_key) override {}

	virtual void set_tag(const String &p_key, const String &p_value) override {}
	virtual void remove_tag(const String &p_key) override {}

	virtual void set_user(const Ref<SentryUser> &p_user) override {}
	virtual void remove_user() override {}

	virtual void add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
			const String &p_type = "default", const Dictionary &p_data = Dictionary()) override {}
	// TODO: Consider adding the following function.
	// virtual void clear_breadcrumbs() = 0;

	virtual void capture_message(const String &p_message, Level p_level, const String &p_logger = "") override {}
	virtual String get_last_event_id() override { return ""; }

	virtual void initialize() override {}
};

} // namespace sentry

#endif // DISABLED_SDK_H
