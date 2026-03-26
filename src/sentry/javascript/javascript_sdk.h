#pragma once

#include "sentry/internal_sdk.h"

using namespace godot;

namespace sentry::javascript {

// Internal SDK utilizing Sentry for JavaScript.
class JavaScriptSDK : public InternalSDK {
private:
	Vector<Ref<SentryAttachment>> file_attachments;

public:
	_FORCE_INLINE_ const Vector<Ref<SentryAttachment>> &get_file_attachments() const { return file_attachments; }
	virtual void set_context(const String &p_key, const Dictionary &p_value) override;
	virtual void remove_context(const String &p_key) override;

	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void remove_tag(const String &p_key) override;

	virtual void set_user(const Ref<SentryUser> &p_user) override;
	virtual void remove_user() override;

	virtual Ref<SentryBreadcrumb> create_breadcrumb() override;
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) override;

	virtual void log(LogLevel p_level, const String &p_body, const Dictionary &p_attributes = Dictionary()) override;

	virtual String capture_message(const String &p_message, Level p_level = sentry::LEVEL_INFO) override;
	virtual String get_last_event_id() override;

	virtual Ref<SentryEvent> create_event() override;
	virtual String capture_event(const Ref<SentryEvent> &p_event) override;

	virtual void capture_feedback(const Ref<SentryFeedback> &p_feedback) override;

	virtual void add_attachment(const Ref<SentryAttachment> &p_attachment) override;
	virtual void clear_attachments() override;

	virtual void metrics_add_count(const String &p_name, int64_t p_value, const Dictionary &p_attributes) override;
	virtual void metrics_add_gauge(const String &p_name, double p_value, const String &p_unit, const Dictionary &p_attributes) override;
	virtual void metrics_add_distribution(const String &p_name, double p_value, const String &p_unit, const Dictionary &p_attributes) override;

	virtual void set_attribute(const String &p_name, const Variant &p_value) override;
	virtual void remove_attribute(const String &p_name) override;

	virtual void set_trace(const String &p_trace_id, const String &p_parent_span_id) override;

	virtual void init() override;
	virtual void close() override;
	virtual bool is_enabled() const override;

	JavaScriptSDK();
	virtual ~JavaScriptSDK() override;
};

} //namespace sentry::javascript
